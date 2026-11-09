
#include <iostream>
#include <sstream>
#include <string>
#include <csignal>

#include <librdkafka/rdkafka.h>
#include <hiredis/hiredis.h>

using namespace std;

volatile sig_atomic_t running = 1;

void stopConsumer(int signal)
{
    running = 0;
}

// Remove spaces around fields, e.g. " 1500.45 " -> "1500.45"
string trim(const string &value)
{
    const string whitespace = " \t\r\n";

    size_t start = value.find_first_not_of(whitespace);

    if (start == string::npos)
        return "";

    size_t end = value.find_last_not_of(whitespace);

    return value.substr(start, end - start + 1);
}

int main()
{
    // 1. Connect to Redis
    redisContext *redis = redisConnect("127.0.0.1", 6379);

    if (redis == nullptr || redis->err)
    {
        cerr << "Redis connection failed: "
             << (redis ? redis->errstr : "Unable to allocate context")
             << endl;

        if (redis != nullptr)
            redisFree(redis);

        return 1;
    }

    cout << "Connected to Redis successfully!" << endl;

    // 2. Configure Kafka
    char errorString[512];

    rd_kafka_conf_t *config = rd_kafka_conf_new();

    if (rd_kafka_conf_set(config, "bootstrap.servers", "localhost:9092",
                          errorString, sizeof(errorString)) != RD_KAFKA_CONF_OK)
    {
        cerr << "Kafka broker configuration failed: "
             << errorString << endl;

        rd_kafka_conf_destroy(config);
        redisFree(redis);
        return 1;
    }

    if (rd_kafka_conf_set(config, "group.id", "market-redis-group",
                          errorString, sizeof(errorString)) != RD_KAFKA_CONF_OK)
    {
        cerr << "Consumer group configuration failed: "
             << errorString << endl;

        rd_kafka_conf_destroy(config);
        redisFree(redis);
        return 1;
    }

    if (rd_kafka_conf_set(config, "auto.offset.reset", "earliest",
                          errorString, sizeof(errorString)) != RD_KAFKA_CONF_OK)
    {
        cerr << "Offset configuration failed: "
             << errorString << endl;

        rd_kafka_conf_destroy(config);
        redisFree(redis);
        return 1;
    }

    // 3. Create Kafka consumer
    rd_kafka_t *consumer = rd_kafka_new(
        RD_KAFKA_CONSUMER, config, errorString, sizeof(errorString));

    if (consumer == nullptr)
    {
        cerr << "Kafka consumer creation failed: "
             << errorString << endl;

        redisFree(redis);
        return 1;
    }

    // 4. Subscribe to market-data
    rd_kafka_topic_partition_list_t *topics =
        rd_kafka_topic_partition_list_new(1);

    rd_kafka_topic_partition_list_add(
        topics, "market-data", RD_KAFKA_PARTITION_UA);

    rd_kafka_resp_err_t subscribeResult =
        rd_kafka_subscribe(consumer, topics);

    rd_kafka_topic_partition_list_destroy(topics);

    if (subscribeResult != RD_KAFKA_RESP_ERR_NO_ERROR)
    {
        cerr << "Kafka subscription failed: "
             << rd_kafka_err2str(subscribeResult) << endl;

        rd_kafka_destroy(consumer);
        redisFree(redis);
        return 1;
    }

    cout << "Subscribed to market-data." << endl;
    cout << "Waiting for market updates..." << endl;

    signal(SIGINT, stopConsumer);

    // 5. Consume Kafka messages and update Redis
    while (running)
    {
        rd_kafka_message_t *message =
            rd_kafka_consumer_poll(consumer, 1000);

        if (message == nullptr)
            continue;

        if (message->err != RD_KAFKA_RESP_ERR_NO_ERROR)
        {
            cerr << "Kafka error: "
                 << rd_kafka_message_errstr(message) << endl;

            rd_kafka_message_destroy(message);
            continue;
        }

        string data(
            static_cast<const char *>(message->payload),
            message->len);

        rd_kafka_message_destroy(message);

        // Expected format: SYMBOL,PRICE,VOLUME
        stringstream ss(data);

        string symbol;
        string price;
        string volume;

        getline(ss, symbol, ',');
        getline(ss, price, ',');
        getline(ss, volume, ',');

        symbol = trim(symbol);
        price = trim(price);
        volume = trim(volume);

        if (symbol.empty() || price.empty() || volume.empty())
        {
            cerr << "Invalid market message: " << data << endl;
            continue;
        }

        // Redis key: stock:INFOSYS, stock:TCS, etc.
        string redisKey = "stock:" + symbol;

        redisReply *reply = static_cast<redisReply *>(
            redisCommand(redis,
                         "HSET %s symbol %s price %s volume %s",
                         redisKey.c_str(),
                         symbol.c_str(),
                         price.c_str(),
                         volume.c_str()));

        if (reply == nullptr)
        {
            cerr << "Redis command failed: " << redis->errstr << endl;
            break;
        }

        if (reply->type == REDIS_REPLY_ERROR)
        {
            cerr << "Redis error: " << reply->str << endl;
        }
        else
        {
            cout << "Updated Redis: "
                 << symbol
                 << " | Price: " << price
                 << " | Volume: " << volume << endl;
        }

        freeReplyObject(reply);
    }

    // 6. Clean up
    cout << "\nClosing consumer..." << endl;

    rd_kafka_consumer_close(consumer);
    rd_kafka_destroy(consumer);

    redisFree(redis);

    cout << "Consumer stopped." << endl;

    return 0;
}
