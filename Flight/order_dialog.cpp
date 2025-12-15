#include "order_dialog.h"
#include "ui_order_dialog.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>
#include <QRandomGenerator>
#include<QFile>
#include <QListWidget>
OrderDialog::~OrderDialog()
{
    delete ui;
}

OrderDialog::OrderDialog(int ticketId, int userId, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OrderDialog)
    , ticketId(ticketId)
    , userId(userId)
    , ticketPrice(0.0)
    , userBalance(0.0)
{
    ui->setupUi(this);
    setWindowTitle("填写订单信息");
    setModal(true);
    ui->comboBox_class->addItem("经济舱");
    ui->comboBox_class->addItem("商务舱");
    connect(ui->comboBox_class, &QComboBox::currentIndexChanged, this, &OrderDialog::calculateTotal);
    loadTicketInfo();
    loadUserBalance();
    ui->spinBox_count->setMinimum(1);
    ui->spinBox_count->setMaximum(10);
    ui->spinBox_count->setValue(1);
    QFile qssFile(":/styles/order_dialog.qss");
    if (qssFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(qssFile.readAll());
        this->setStyleSheet(styleSheet);  // 只影响当前窗口
        qssFile.close();
        qDebug()<<"成功读取文件";
    }
}

void OrderDialog::loadTicketInfo()
{
    if (!QSqlDatabase::database().isOpen()) {
        QMessageBox::warning(this, "错误", "数据库未连接！");
        return;
    }

    QSqlQuery query;
    query.prepare("SELECT flight_id, flight_number, departure_city, arrival_city, departure_time, "
                  "arrival_time, price, departure_airport, arrival_airport, airline_company, availableSeat "
                  "FROM flight_Info WHERE flight_id = ?");
    query.addBindValue(ticketId);

    if (!query.exec() || !query.next()) {
        QMessageBox::critical(this, "错误", "获取票务信息失败！");
        reject();
        return;
    }


    //暂时保留此处类型，默认为航班
    QString typeName ="航班";
    // QString ticketType = query.value(0).toString();
    // QString typeName = ticketType == "Flight" ? "航班" : (ticketType == "Train" ? "火车" : "汽车");

    ui->label_type->setText(typeName);
    ui->label_ticketNo->setText(query.value(1).toString());
    ui->label_route->setText(query.value(2).toString() + query.value(7).toString() + " → " + query.value(3).toString() + query.value(8).toString());

    QDateTime depTime = query.value(4).toDateTime();
    QDateTime arrTime = query.value(5).toDateTime();
    ui->label_departure->setText(depTime.toString("yyyy-MM-dd hh:mm"));
    ui->label_arrival->setText(arrTime.toString("yyyy-MM-dd hh:mm"));

    ticketPrice = query.value(6).toDouble();
    ui->label_price->setText(QString::number(ticketPrice, 'f', 2) + " 元");

    ui->label_available->setText(QString::number(query.value(10).toInt()) + " 张");
    ui->label_company->setText(query.value(9).toString());

    ui->spinBox_count->setMaximum(query.value(10).toInt());
    calculateTotal();
}

void OrderDialog::on_spinBox_count_valueChanged(int count)
{
    Q_UNUSED(count);
    calculateTotal();
}

void OrderDialog::calculateTotal()
{
    int count = ui->spinBox_count->value();
    double currentPrice = ticketPrice;
    if (ui->comboBox_class->currentText() == "商务舱") {
        currentPrice += 200.0; // 商务舱加价200
    }
    double total = currentPrice * count;
    ui->label_total->setText(QString::number(total, 'f', 2) + " 元");
    if(ui->label_balance){
        if(userBalance<total){
            ui->label_balance->setStyleSheet("color: red; font-weight: bold;");
        }else{
            ui->label_balance->setStyleSheet("color: green; font-weight: bold;");
        }
    }
}

//检测时间是否冲突
bool OrderDialog::checkTimeConflict(const QString&passengerIDCard,int newTicketId){
    if(passengerIDCard.isEmpty()){
        return false;
    }
    QSqlQuery newTicketQuery;
    newTicketQuery.prepare("SELECT departure_time, arrival_time FROM flight_info WHERE flight_id = ?");
    newTicketQuery.addBindValue(newTicketId);
    if (!newTicketQuery.exec() || !newTicketQuery.next()) {
        qDebug() << "无法获取新票务信息";
        return false;
    }
    QDateTime newDepTime = newTicketQuery.value(0).toDateTime();
    QDateTime newArrTime = newTicketQuery.value(1).toDateTime();
    //获取这个乘客未出行的订单
    QSqlQuery query;
    query.prepare("SELECT t.departure_time, t.arrival_time, t.flight_number "
                  "FROM orders o "
                  "JOIN flight_info t ON o.TicketID = t.flight_id "
                  "WHERE o.PassengerIDCard = ? AND o.OrderStatus IN ('Pending', 'Confirmed', 'Paid') "
                  "AND t.departure_time > ?");
    query.addBindValue(passengerIDCard);
    query.addBindValue(QDateTime::currentDateTime());
    if (!query.exec()) {
        qDebug() << "查询乘客订单失败:" << query.lastError().text();
        return false;
    }
    while (query.next()) {
        QDateTime existingDepTime = query.value(0).toDateTime();
        QDateTime existingArrTime = query.value(1).toDateTime();

        // 时间冲突判断逻辑
        bool timeOverlap = (newDepTime < existingArrTime && newArrTime > existingDepTime);

        if (timeOverlap) {
            qDebug() << "检测到时间冲突: 新票" << newDepTime << "-" << newArrTime
                     << "与已有票" << existingDepTime << "-" << existingArrTime << "重叠";
            return true;
        }

        // 检查换乘时间是否足够（最小30分钟）
        int minTransferTime = 30; // 分钟

        // 新票在已有票之后，检查换乘时间
        if (newDepTime > existingArrTime) {
            qint64 transferSeconds = existingArrTime.secsTo(newDepTime);
            if (transferSeconds < minTransferTime * 60) {
                qDebug() << "换乘时间不足:" << transferSeconds / 60 << "分钟";
                query.finish();
                return true;
            }
        }

        // 已有票在新票之后，检查换乘时间
        if (existingDepTime > newArrTime) {
            qint64 transferSeconds = newArrTime.secsTo(existingDepTime);
            if (transferSeconds < minTransferTime * 60) {
                qDebug() << "换乘时间不足:" << transferSeconds / 60 << "分钟";
                query.finish();
                return true;
            }
        }
    }
    return false;

}
//获取冲突
QString OrderDialog:: getConflictDetails(const QString &passengerIDCard, int newTicketId)
{
    if (passengerIDCard.isEmpty()) {
        return "";
    }

    // 获取新票信息
    QSqlQuery newTicketQuery;
    newTicketQuery.prepare("SELECT departure_time, arrival_time FROM flight_info WHERE flight_id = ?");
    newTicketQuery.addBindValue(newTicketId);

    if (!newTicketQuery.exec() || !newTicketQuery.next()) {
        return "";
    }

    QDateTime newDepTime = newTicketQuery.value(0).toDateTime();
    QDateTime newArrTime = newTicketQuery.value(1).toDateTime();

    // 查找冲突的行程
    QSqlQuery query;
    query.prepare("SELECT t.flight_number, t.departure_city, t.arrival_city, t.departure_airport, t.arrival_airport, "
                  "t.departure_time, t.arrival_time, t.airline_company "
                  "FROM orders o "
                  "JOIN flight_info t ON o.TicketID = t.flight_id "
                  "WHERE o.PassengerIDCard = ? AND o.OrderStatus IN ('Pending', 'Confirmed', 'Paid') "
                  "AND t.departure_time > ?");

    query.addBindValue(passengerIDCard);
    query.addBindValue(QDateTime::currentDateTime());

    if (!query.exec()) {
        return "";
    }

    while (query.next()) {
        QDateTime existingDepTime = query.value(5).toDateTime();
        QDateTime existingArrTime = query.value(6).toDateTime();

        // 检查时间冲突
        bool timeOverlap = (newDepTime < existingArrTime && newArrTime > existingDepTime);

        // 检查换乘时间
        bool insufficientTransfer = false;
        if (newDepTime > existingArrTime) {
            qint64 transferSeconds = existingArrTime.secsTo(newDepTime);
            insufficientTransfer = (transferSeconds < 30 * 60);
        } else if (existingDepTime > newArrTime) {
            qint64 transferSeconds = newArrTime.secsTo(existingDepTime);
            insufficientTransfer = (transferSeconds < 30 * 60);
        }

        if (timeOverlap || insufficientTransfer) {
            QString ticketType = "Flight";
            QString typeName = ticketType == "Flight" ? "航班" : (ticketType == "Train" ? "火车" : "汽车");

            return QString("%1 %2\n%3%4 → %5%6\n%7 - %8\n运营商：%9")
                .arg(typeName)
                .arg(query.value(0).toString())
                .arg(query.value(1).toString())
                .arg(query.value(3).toString())
                .arg(query.value(2).toString())
                .arg(query.value(4).toString())
                .arg(existingDepTime.toString("MM-dd hh:mm"))
                .arg(existingArrTime.toString("MM-dd hh:mm"))
                .arg(query.value(7).toString());
        }
    }

    return "";
}

void OrderDialog::on_btn_confirm_clicked()
{
    QString passengerName = ui->lineEdit_name->text().trimmed();
    QString passengerID = ui->lineEdit_id->text().trimmed();
    QString contactPhone = ui->lineEdit_phone->text().trimmed();
    int count = ui->spinBox_count->value();
    //检测金额是否超出余额
    loadUserBalance();
    double TotalCost=ticketPrice*count;
    if(TotalCost>userBalance){
        QMessageBox::critical(this, "支付失败",
                              QString("您的余额不足！\n当前余额: ¥%1\n订单金额: ¥%2\n请先充值。")
                                  .arg(QString::number(userBalance, 'f', 2))
                                  .arg(QString::number(TotalCost, 'f', 2)));
        return;
    }
    if (passengerName.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入乘客姓名！");
        return;
    }
    if (contactPhone.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入联系电话！");
        return;
    }

    if (passengerID.length() != 18) {
        QMessageBox::warning(this, "提示", "请输入18位身份证号码！");
        ui->lineEdit_id->setFocus();
        ui->lineEdit_id->selectAll();
        return;
    }

    if (!QSqlDatabase::database().isOpen()) {
        QMessageBox::warning(this, "错误", "数据库未连接！");
        return;
    }

    //行程是否冲突
    if (checkTimeConflict(passengerID, ticketId)) {
        QString conflictDetails = getConflictDetails(passengerID, ticketId);

        QString message = QString("检测到行程时间冲突！\n\n乘客：%1 (身份证：%2)\n\n").arg(passengerName,passengerID);

        if (!conflictDetails.isEmpty()) {
            message += QString("冲突的行程：\n%1\n\n").arg(conflictDetails);
        }

        // 获取当前票务信息
        QSqlQuery ticketQuery;
        ticketQuery.prepare("SELECT flight_number, departure_city, departure_airport, arrival_city, arrival_airport, departure_time, arrival_time FROM flight_info WHERE flight_id = ?");
        ticketQuery.addBindValue(ticketId);

        if (ticketQuery.exec() && ticketQuery.next()) {
            message += QString("您要预订的行程：\n%1 %2\n%3%4 → %5%6\n%7 - %8")
                           .arg("航班")
                           .arg(ticketQuery.value(0).toString())
                           .arg(ticketQuery.value(1).toString())
                           .arg(ticketQuery.value(2).toString())
                           .arg(ticketQuery.value(3).toString())
                           .arg(ticketQuery.value(4).toString())
                           .arg(ticketQuery.value(5).toDateTime().toString("MM-dd hh:mm"))
                           .arg(ticketQuery.value(6).toDateTime().toString("MM-dd hh:mm"));
        }

        QMessageBox::warning(this, "行程冲突", message);
        return;
    }

    // 检查可用座位数
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT availableSeat FROM flight_info WHERE flight_id = ?");
    checkQuery.addBindValue(ticketId);
    if (!checkQuery.exec() || !checkQuery.next()) {
        QMessageBox::warning(this, "错误", "检查座位失败！");
        return;
    }
    int availableSeat = checkQuery.value(0).toInt();
    if (availableSeat < count) {
        QMessageBox::warning(this, "提示", QString("可用座位不足！当前可用：%1 张").arg(availableSeat));
        return;
    }

    // 生成订单号
    QString orderNo = "ORD" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss") +
                      QString::number(QRandomGenerator::global()->bounded(1000000), 10).rightJustified(6, '0');

    // 开始事务
    db.transaction();

    try {
        // 插入订单
        QSqlQuery insertQuery(db);
        insertQuery.prepare("INSERT INTO orders (UserID, TicketID, OrderNo, PassengerName, "
                            "PassengerIDCard, ContactPhone, TicketCount, TotalPrice, OrderStatus,CabinClass) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, 'Paid',?)");
        QString cabinClass = ui->comboBox_class->currentText();
        insertQuery.addBindValue(userId);
        insertQuery.addBindValue(ticketId);
        insertQuery.addBindValue(orderNo);
        insertQuery.addBindValue(passengerName);
        insertQuery.addBindValue(passengerID);
        insertQuery.addBindValue(contactPhone);
        insertQuery.addBindValue(count);
        double currentPrice = ticketPrice;
        if (cabinClass == "商务舱") currentPrice += 200.0;
        insertQuery.addBindValue(currentPrice * count);

        insertQuery.addBindValue(cabinClass);

        if (!insertQuery.exec()) {
            db.rollback();
            QMessageBox::critical(this, "错误", "创建订单失败：" + insertQuery.lastError().text());
            return;
        }

        // 更新票务可用座位数
        QSqlQuery updateQuery(db);
        updateQuery.prepare("UPDATE flight_info SET availableSeat = availableSeat - ? WHERE flight_id = ?");
        updateQuery.addBindValue(count);
        updateQuery.addBindValue(ticketId);

        if (!updateQuery.exec()) {
            db.rollback();
            QMessageBox::critical(this, "错误", "更新座位失败：" + updateQuery.lastError().text());
            return;
        }

        QSqlQuery deductQuery(db);
        deductQuery.prepare("UPDATE users SET Balance = Balance - ? WHERE UserID = ?");
        deductQuery.addBindValue(TotalCost);
        deductQuery.addBindValue(userId);
        if (!deductQuery.exec()) {
            throw deductQuery.lastError(); // 抛出异常触发回滚
        }

        // 提交事务
        db.commit();
        QMessageBox::information(this, "成功", QString("订单创建成功！\n订单号：%1").arg(orderNo));
        accept();
    } catch (...) {
        db.rollback();
        QMessageBox::critical(this, "错误", "订票过程中发生错误！");
    }
}

void OrderDialog::on_btn_cancel_clicked()
{
    reject();
}
void OrderDialog::loadUserBalance()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;

    QSqlQuery query;
    query.prepare("SELECT Balance FROM users WHERE UserID = ?");
    query.addBindValue(userId);

    if (query.exec() && query.next()) {
        userBalance = query.value(0).toDouble();
        if (ui->label_balance) {
            ui->label_balance->setText(QString("当前余额: ¥ %1").arg(QString::number(userBalance, 'f', 2)));
            double currentTotal = ticketPrice * ui->spinBox_count->value();
            if (userBalance < currentTotal) {
                ui->label_balance->setStyleSheet("color: red; font-weight: bold;");
            } else {
                ui->label_balance->setStyleSheet("color: green; font-weight: bold;");
            }
        }
    } else {
        qDebug() << "查询余额失败:" << query.lastError().text();
    }
}
void OrderDialog::on_btn_passenger_clicked()
{
    if (!QSqlDatabase::database().isOpen()) {
        QMessageBox::warning(this, "错误", "数据库未连接！");
        return;
    }

    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle("选择乘机人");
    dlg->resize(350, 450);

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    QListWidget *listWidget = new QListWidget(dlg);

    listWidget->setStyleSheet(
        "QListWidget { border: 1px solid #ccc; outline: none; }"
        "QListWidget::item { padding: 10px; border-bottom: 1px solid #eee; }"
        "QListWidget::item:hover { background-color: #f5f5f5; }"
        "QListWidget::item:selected { background-color: #e6f7ff; color: #333; }"
        "QListWidget::indicator { width: 20px; height: 20px; }"
        );
    layout->addWidget(listWidget);

    QSqlQuery query;
    query.prepare("SELECT Name, IDCard, Phone FROM passengers WHERE UserID = ?");
    query.addBindValue(this->userId);

    bool hasData = false;
    if (query.exec()) {
        while (query.next()) {
            hasData = true;
            QString name = query.value(0).toString();
            QString idCard = query.value(1).toString();
            QString phone = query.value(2).toString();

            QListWidgetItem *item = new QListWidgetItem(name);

            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);

            item->setData(Qt::UserRole, idCard);
            item->setData(Qt::UserRole + 1, phone);

            listWidget->addItem(item);
        }
    }

    if (!hasData) {
        QMessageBox::information(this, "提示", "您还没有添加常用乘机人，请先去个人中心添加。");
        delete dlg;
        return;
    }

    connect(listWidget, &QListWidget::itemClicked, dlg, [listWidget](QListWidgetItem *item){

        listWidget->blockSignals(true);
        for(int i = 0; i < listWidget->count(); ++i) {
            QListWidgetItem *it = listWidget->item(i);
            it->setCheckState(Qt::Unchecked);
            it->setSelected(false);
        }
        item->setCheckState(Qt::Checked);
        item->setSelected(true);
        listWidget->blockSignals(false);
    });

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnOk = new QPushButton("确定", dlg);
    QPushButton *btnCancel = new QPushButton("取消", dlg);

    btnOk->setStyleSheet("background-color: #4CAF50; color: white; border-radius: 4px; padding: 6px 15px;");
    btnCancel->setStyleSheet("background-color: #f44336; color: white; border-radius: 4px; padding: 6px 15px;");

    btnLayout->addStretch();
    btnLayout->addWidget(btnOk);
    btnLayout->addWidget(btnCancel);
    layout->addLayout(btnLayout);

    connect(btnOk, &QPushButton::clicked, dlg, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, dlg, &QDialog::reject);

    connect(listWidget, &QListWidget::itemDoubleClicked, dlg, [dlg, listWidget](QListWidgetItem *item){
        item->setCheckState(Qt::Checked);
        dlg->accept();
    });

    if (dlg->exec() == QDialog::Accepted) {
        QListWidgetItem *selectedItem = nullptr;
        for(int i = 0; i < listWidget->count(); ++i) {
            if (listWidget->item(i)->checkState() == Qt::Checked) {
                selectedItem = listWidget->item(i);
                break;
            }
        }

        if (selectedItem) {
            ui->lineEdit_name->setText(selectedItem->text());
            ui->lineEdit_id->setText(selectedItem->data(Qt::UserRole).toString());
            ui->lineEdit_phone->setText(selectedItem->data(Qt::UserRole + 1).toString());
        } else {
            QMessageBox::warning(this, "提示", "未选择任何乘机人！");
        }
    }
    delete dlg;
}
