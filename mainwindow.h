#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

struct redisContext;
class QTimer;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    Ui::MainWindow *ui;

    // Redis connection
    struct redisContext *redis = nullptr;

    // Timer for periodic updates
    QTimer *refreshTimer = nullptr;

    // Fetch latest stock data
    void refreshStockData();
};

#endif // MAINWINDOW_H