#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QTimer>

#include <hiredis/hiredis.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Configure stock table
    ui->stockTable->setColumnCount(3);
    ui->stockTable->setHorizontalHeaderLabels(
        {"Symbol", "Price", "Volume"});

    ui->stockTable->setRowCount(2);

    ui->stockTable->setAlternatingRowColors(true);
    ui->stockTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);
    ui->stockTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    ui->stockTable->verticalHeader()->setVisible(false);
    ui->stockTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);

    ui->stockTable->setMinimumHeight(250);
    ui->stockTable->setShowGrid(true);
    ui->stockTable->setFocusPolicy(Qt::NoFocus);

    // Connect to Redis
    redis = redisConnect("127.0.0.1", 6379);

    if (redis == nullptr || redis->err)
    {
        ui->redisStatusLabel->setText("Redis: Disconnected");

        if (redis != nullptr)
        {
            redisFree(redis);
            redis = nullptr;
        }

        return;
    }

    ui->redisStatusLabel->setText("Redis: Connected");

    // Create timer
    refreshTimer = new QTimer(this);

    connect(refreshTimer, &QTimer::timeout,
            this, &MainWindow::refreshStockData);

    // Fetch immediately, then every second
    refreshStockData();
    refreshTimer->start(1000);
}

void MainWindow::refreshStockData()
{
    const char* symbols[] = {"INFOSYS", "TCS"};

    for (int row = 0; row < 2; row++)
    {
        QString symbol;
        QString price;
        QString volume;

        std::string redisKey =
            "stock:" + std::string(symbols[row]);

        redisReply *reply = static_cast<redisReply*>(
            redisCommand(redis, "HGETALL %s", redisKey.c_str())
            );

        if (reply == nullptr)
            continue;

        if (reply->type == REDIS_REPLY_ARRAY)
        {
            for (size_t i = 0; i + 1 < reply->elements; i += 2)
            {
                QString field = QString::fromUtf8(
                    reply->element[i]->str,
                    static_cast<int>(reply->element[i]->len));

                QString value = QString::fromUtf8(
                    reply->element[i + 1]->str,
                    static_cast<int>(reply->element[i + 1]->len));

                if (field == "symbol")
                    symbol = value;
                else if (field == "price")
                    price = value;
                else if (field == "volume")
                    volume = value;
            }
        }

        freeReplyObject(reply);

        if (symbol.isEmpty())
            continue;

        QString values[] = {symbol, price, volume};

        for (int col = 0; col < 3; col++)
        {
            QTableWidgetItem *item =
                ui->stockTable->item(row, col);

            if (item == nullptr)
            {
                item = new QTableWidgetItem();
                ui->stockTable->setItem(row, col, item);
            }

            item->setText(values[col]);
        }
    }
}

MainWindow::~MainWindow()
{
    if (refreshTimer != nullptr)
        refreshTimer->stop();

    if (redis != nullptr)
        redisFree(redis);

    delete ui;
}