#include "single_center.h"
#include "ui_single_center.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDebug>
#include <QPushButton>
#include <QHeaderView>
#include <QDateTime>
#include <QTableWidgetItem>
#include<QFile>
Single_Center::Single_Center(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Single_Center)
{
    ui->setupUi(this);
    currentUsername = "";
    initTable();
    QFile qssFile(":/styles/single.qss");
    if (qssFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(qssFile.readAll());
        this->setStyleSheet(styleSheet);  // 只影响当前窗口
        qssFile.close();
        qDebug()<<"成功读取文件";
    }
}

Single_Center::Single_Center(const QString &username, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Single_Center)
{
    ui->setupUi(this);
    currentUsername = username;
    initTable();
    loadOrders();
    connect(ui->btn_refresh, &QPushButton::clicked, this, &Single_Center::refreshOrderList);
    connect(ui->btn_back, &QPushButton::clicked, this, &Single_Center::on_btn_back_clicked);
    QFile qssFile(":/styles/single.qss");
    if (qssFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(qssFile.readAll());
        this->setStyleSheet(styleSheet);  // 只影响当前窗口
        qssFile.close();
        qDebug()<<"成功读取文件";
    }
}

Single_Center::~Single_Center()
{
    delete ui;
}
void Single_Center::on_btn_back_clicked()
{
    emit backRequested(); // 发出"返回请求"信号
}
void Single_Center::initTable()
{
    QStringList headers;
    headers << "订单号" << "航班号" << "路线" << "出发时间" << "到达时间"
            << "数量" << "总价(元)" << "状态" << "操作"<< "收藏";

    ui->tableWidget_orders->setColumnCount(headers.size());
    ui->tableWidget_orders->setHorizontalHeaderLabels(headers);

    QHeaderView* header = ui->tableWidget_orders->horizontalHeader();

    //设置默认模式为 Stretch，确保所有列能填满整个页面宽度
    header->setSectionResizeMode(QHeaderView::Stretch);

    //根据内容自动收缩宽度
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents); // 订单号
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents); // 航班
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents); // 出发时间
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents); // 到达时间
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents); // 数量
    header->setSectionResizeMode(6, QHeaderView::ResizeToContents); // 总价
    header->setSectionResizeMode(7, QHeaderView::ResizeToContents); // 状态
    header->setSectionResizeMode(8, QHeaderView::ResizeToContents); // 操作
    header->setSectionResizeMode(9, QHeaderView::ResizeToContents); // 收藏
    ui->tableWidget_orders->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_orders->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_orders->verticalHeader()->setVisible(false);
}

int Single_Center::getUserId()
{
    if (currentUsername.isEmpty()) {
        return -1;
    }
    bool ok;
    int userId = currentUsername.toInt(&ok);
    if (ok && userId > 0) {
        return userId;
    }
    return -1;
}

void Single_Center::loadOrders()
{
    int userId = getUserId();
    if (userId == -1) {
        QMessageBox::warning(this, "错误", "获取用户信息失败！");
        return;
    }

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        QMessageBox::warning(this, "错误", "数据库未连接！");
        return;
    }

    QList<int> favoriteTicketIds;
    if (userId!=-1) {
        QSqlQuery favQuery;
        favQuery.prepare("SELECT TicketID FROM favorites WHERE UserID = :uid");
        favQuery.bindValue(":uid", userId);
        if (favQuery.exec()) {
            while(favQuery.next()) {
                favoriteTicketIds.append(favQuery.value(0).toInt());
            }
        }
        favQuery.finish();
    }

    QSqlQuery query(db);  // 显式指定数据库连接
    query.prepare("SELECT o.OrderID, o.OrderNo, o.OrderStatus, o.TicketCount, o.TotalPrice, "
                  "o.OrderTime, t.departure_city, t.departure_airport, t.arrival_city, t.arrival_airport, "
                  "t.departure_time, t.arrival_time, t.flight_number, t.flight_id "
                  "FROM orders o "
                  "JOIN flight_info t ON o.TicketID = t.flight_id "
                  "WHERE o.UserID = ? "
                  "ORDER BY o.OrderTime DESC");
    query.addBindValue(userId);

    if (!query.exec()) {
        QMessageBox::critical(this, "错误", "查询订单失败：" + query.lastError().text());
        return;
    }
    ui->tableWidget_orders->setRowCount(0);
    int row = 0;
    while (query.next()) {
        ui->tableWidget_orders->insertRow(row);

        int orderId = query.value(0).toInt();
        QString orderNo = query.value(1).toString();
        QString status = query.value(2).toString();
        int count = query.value(3).toInt();
        double totalPrice = query.value(4).toDouble();
        QString route = query.value(6).toString() + query.value(7).toString() + " → " + query.value(8).toString() + query.value(9).toString();
        QDateTime depTime = query.value(10).toDateTime();
        QDateTime arrTime = query.value(11).toDateTime();

        QString typeName = query.value(12).toString();
        QString statusName = status == "Paid" ? "已支付" : (status == "Cancelled" ? "已取消" : "待支付");

        //判断订单是否过期，是否支付
        if (status == "Paid") {
            if (arrTime < QDateTime::currentDateTime()) {
                statusName = "已过期";
            } else {
                statusName = "已支付";
            }
        } else if (status == "Cancelled") {
            statusName = "已取消";
        } else {
            if (depTime < QDateTime::currentDateTime()) {
                statusName = "已过期";
            } else {
                statusName = "待支付";
            }
        }

        ui->tableWidget_orders->setItem(row, 0, new QTableWidgetItem(orderNo));
        ui->tableWidget_orders->setItem(row, 1, new QTableWidgetItem(typeName));
        ui->tableWidget_orders->setItem(row, 2, new QTableWidgetItem(route));
        ui->tableWidget_orders->setItem(row, 3, new QTableWidgetItem(depTime.toString("yyyy-MM-dd hh:mm")));
        ui->tableWidget_orders->setItem(row, 4, new QTableWidgetItem(arrTime.toString("yyyy-MM-dd hh:mm")));
        ui->tableWidget_orders->setItem(row, 5, new QTableWidgetItem(QString::number(count)));
        ui->tableWidget_orders->setItem(row, 6, new QTableWidgetItem(QString::number(totalPrice, 'f', 2)));
        ui->tableWidget_orders->setItem(row, 7, new QTableWidgetItem(statusName));

        // 添加取消订单按钮（只有已支付的订单可以取消）
        if (status == "Paid" && arrTime > QDateTime::currentDateTime()) {
            QPushButton *btnCancel = new QPushButton("取消订单");
            btnCancel->setProperty("orderId", orderId);
            connect(btnCancel, &QPushButton::clicked, this, &Single_Center::onCancelOrder);
            ui->tableWidget_orders->setCellWidget(row, 8, btnCancel);

        } else if(status == "Paid" && arrTime < QDateTime::currentDateTime()) {
            QPushButton *btndelete = new QPushButton("删除订单");
            btndelete->setProperty("orderId", orderId);
            connect(btndelete, &QPushButton::clicked, this, &Single_Center::onDeleteOrder);
            ui->tableWidget_orders->setCellWidget(row, 8, btndelete);

        } else {
            ui->tableWidget_orders->setItem(row, 8, new QTableWidgetItem("-"));
        }

        int ticketId=query.value("flight_id").toInt();
        bool isFavorited = favoriteTicketIds.contains(ticketId);
        QPushButton *btnFav = new QPushButton(isFavorited ? "已收藏" : "收藏");
        btnFav->setProperty("ticketId", ticketId);

        if (isFavorited) {
            btnFav->setEnabled(false);
        }
        connect(btnFav, &QPushButton::clicked, this, &Single_Center::onAddFavorite);
        ui->tableWidget_orders->setCellWidget(row, 9, btnFav);

        row++;
    }

    query.finish();
}

void Single_Center::refreshOrderList()
{
    loadOrders();
}

void Single_Center::onCancelOrder()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int orderId = btn->property("orderId").toInt();

    int ret = QMessageBox::question(this, "确认", "确定要取消这个订单吗？",
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }

    QSqlDatabase db = QSqlDatabase::database(); // 移动到外面
    if (!db.isOpen()) {
        QMessageBox::warning(this, "错误", "数据库未连接！");
        return;
    }

    // 获取订单信息
    QSqlQuery orderQuery(db);
    orderQuery.prepare("SELECT TicketID, TicketCount, TotalPrice, UserID FROM orders WHERE OrderID = ?");
    orderQuery.addBindValue(orderId);

    if (!orderQuery.exec() || !orderQuery.next()) {
        QMessageBox::warning(this, "错误", "获取订单信息失败！");
        return;
    }
    int ticketId = orderQuery.value(0).toInt();
    int ticketCount = orderQuery.value(1).toInt();
    double refundAmount = orderQuery.value(2).toDouble();
    int userId = orderQuery.value(3).toInt();
    orderQuery.finish();
    // 开始事务
    db.transaction();

    try {
        // 更新订单状态
        QSqlQuery updateOrderQuery(db);
        updateOrderQuery.prepare("UPDATE orders SET OrderStatus = 'Cancelled' WHERE OrderID = ?");
        updateOrderQuery.addBindValue(orderId);
        if (!updateOrderQuery.exec()) {
            db.rollback();
            QMessageBox::critical(this, "错误", "取消订单失败：" + updateOrderQuery.lastError().text());
            return;
        }

        // 恢复座位数
        QSqlQuery updateTicketQuery(db);
        updateTicketQuery.prepare("UPDATE flight_info SET availableSeat = availableSeat + ? WHERE flight_id = ?");
        updateTicketQuery.addBindValue(ticketCount);
        updateTicketQuery.addBindValue(ticketId);
        if (!updateTicketQuery.exec()) {
            db.rollback();
            QMessageBox::critical(this, "错误", "恢复座位失败：" + updateTicketQuery.lastError().text());
            return;
        }

        QSqlQuery refundQuery(db);
        refundQuery.prepare("UPDATE users SET Balance = Balance + ? WHERE UserID = ?");
        refundQuery.addBindValue(refundAmount);
        refundQuery.addBindValue(userId);
        if (!refundQuery.exec()) throw refundQuery.lastError();
        // 提交事务
        if (!db.commit()) {
            db.rollback(); // 提交失败则回滚
            QMessageBox::critical(this, "错误", "提交事务失败：" + db.lastError().text());
            return;
        }else {
            QMessageBox::information(this, "成功", "订单已取消！订单金额已按原路返回！");
            refreshOrderList();
            emit dataChanged(); // 发射信号通知外部刷新
        }
    } catch (...) {
        db.rollback();
        QMessageBox::critical(this, "错误", "取消订单过程中发生错误！");
    }
}

//删除按钮
void Single_Center::onDeleteOrder()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int orderId = btn->property("orderId").toInt();

    int ret = QMessageBox::question(this, "确认", "确定要删除这个订单吗？",
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        QMessageBox::warning(this, "错误", "数据库未连接！");
        return;
    }

    // 获取订单信息
    QSqlQuery orderQuery(db);
    orderQuery.prepare("SELECT TicketID, TicketCount FROM orders WHERE OrderID = ?");
    orderQuery.addBindValue(orderId);
    if (!orderQuery.exec() || !orderQuery.next()) {
        QMessageBox::warning(this, "错误", "获取订单信息失败！");
        return;
    }
    orderQuery.finish();
    // 开始事务
    db.transaction();

    try {
        // 更新订单状态
        QSqlQuery updateOrderQuery(db);
        updateOrderQuery.prepare("UPDATE orders SET OrderStatus = 'Cancelled' WHERE OrderID = ?");
        updateOrderQuery.addBindValue(orderId);
        if (!updateOrderQuery.exec()) {
            db.rollback();
            QMessageBox::critical(this, "错误", "删除订单失败：" + updateOrderQuery.lastError().text());
            return;
        }

        // 提交事务
        db.commit();
        QMessageBox::information(this, "成功", "订单已删除！");
        refreshOrderList();
    } catch (...) {
        db.rollback();
        QMessageBox::critical(this, "错误", "删除订单过程中发生错误！");
    }
}

void Single_Center::onAddFavorite()
{
    int currentUserID=getUserId();
    if (currentUserID==-1) {
        QMessageBox::warning(this, "提示", "请先登录！");
        return;
    }
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    int ticketId = btn->property("ticketId").toInt();

    QSqlQuery insertQuery;
    insertQuery.prepare("INSERT INTO favorites (UserID, TicketID) VALUES (?, ?)");
    insertQuery.addBindValue(currentUserID);
    insertQuery.addBindValue(ticketId);

    if (insertQuery.exec()) {
        QMessageBox::information(this, "成功", "已添加到收藏夹！");
        btn->setEnabled(false);
        btn->setText("已收藏");
    } else {
        if (insertQuery.lastError().text().contains("Duplicate") ||
            insertQuery.lastError().text().contains("UNIQUE")) {
            QMessageBox::information(this, "提示", "您已经收藏过该行程了！");
        } else {
            QMessageBox::critical(this, "错误", "收藏失败：" + insertQuery.lastError().text());
        }
    }
    insertQuery.finish();
}
void Single_Center::onViewOrder()
{

}
