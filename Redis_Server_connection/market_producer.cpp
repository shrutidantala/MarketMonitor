
#include <iostream>
#include <cstring>
#include <string>
#include <random>
#include <sstream>
#include <iomanip>

#include <librdkafka/rdkafka.h>

using namespace std;

int main()
{
    char errorString[512];

    // 1. Create Kafka configuration
    rd_kafka_conf_t *config = rd_kafka_conf_new();

    // 2. Configure Kafka broker address
    if (rd_kafka_conf_set(
            config,
            "bootstrap.servers",
            "localhost:9092",
            errorString,
            sizeof(errorString)) != RD_KAFKA_CONF_OK)
    {
        cerr << "Configuration error: "
             << errorString << endl;

        rd_kafka_conf_destroy(config);
        return 1;
    }

    // 3. Create Kafka producer
    rd_kafka_t *producer = rd_kafka_new(
        RD_KAFKA_PRODUCER,
        config,
        errorString,
        sizeof(errorString));

    if (producer == nullptr)
    {
        cerr << "Producer creation failed: "
             << errorString << endl;
        return 1;
    }

    cout << "Kafka producer created successfully!" << endl;

    // 4. Select the topic
    rd_kafka_topic_t *topic = rd_kafka_topic_new(
        producer,
        "market-data",
        nullptr);

    if (topic == nullptr)
    {
        cerr << "Topic creation failed: "
             << rd_kafka_err2str(rd_kafka_last_error())
             << endl;

        rd_kafka_destroy(producer);
        return 1;
    }

    // 5. Generate random market data
    random_device rd;
    mt19937 gen(rd());

    uniform_real_distribution<double> infosysPrice(1400.0, 1800.0);
    uniform_real_distribution<double> tcsPrice(3000.0, 4000.0);

    uniform_int_distribution<int> volumeRange(5000, 50000);

    // Stocks to publish
    string symbols[] = {"INFOSYS", "TCS"};

    for (int i = 0; i < 2; i++)
    {
        double price;

        if (symbols[i] == "INFOSYS")
            price = infosysPrice(gen);
        else
            price = tcsPrice(gen);

        int volume = volumeRange(gen);

        // Prepare market data message
        ostringstream stream;

        stream << symbols[i] << ","
               << fixed << setprecision(2) << price
               << "," << volume;

        string message = stream.str();

        // 6. Publish message to Kafka
        if (rd_kafka_produce(
                topic,
                RD_KAFKA_PARTITION_UA,
                RD_KAFKA_MSG_F_COPY,
                (void *)message.c_str(),
                message.length(),
                nullptr,
                0,
                nullptr) == -1)
        {
            cerr << "Message enqueue failed: "
                 << rd_kafka_err2str(rd_kafka_last_error())
                 << endl;
        }
        else
        {
            cout << "Message queued: " << message << endl;
        }
    }

    // 7. Wait for delivery reports and process events
    rd_kafka_flush(producer, 10000);

    // 8. Clean up resources
    rd_kafka_topic_destroy(topic);
    rd_kafka_destroy(producer);

    cout << "Producer finished." << endl;

    return 0;
}
