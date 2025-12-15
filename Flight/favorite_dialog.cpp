#include "favorite_dialog.h"
#include "ui_favorite_dialog.h"
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
favorite_dialog::favorite_dialog(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::favorite_dialog)
{
    ui->setupUi(this);
    QFile qssFile(":/styles/favorite.qss");
    if (qssFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(qssFile.readAll());
        this->setStyleSheet(styleSheet);  // 只影响当前窗口
        qssFile.close();
        qDebug()<<"成功读取文件";
    }
}

favorite_dialog::favorite_dialog(const QString &userID, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::favorite_dialog)
    , currentUserID(userID)
{
    ui->setupUi(this);
    initTable();
    loadFavorites(); // 构造时加载数据
    QFile qssFile(":/styles/favorite.qss");
    if (qssFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(qssFile.readAll());
        this->setStyleSheet(styleSheet);  // 只影响当前窗口
        qssFile.close();
        qDebug()<<"成功读取文件";
    }
}

favorite_dialog::~favorite_dialog()
{
    delete ui;
}

void favorite_dialog::initTable()
{
    QStringList headers;
    headers << "类型" << "编号" << "出发地" << "目的地"
            << "出发时间" << "到达时间" << "价格（元）" << "公司" << "收藏";
    ui->tableWidget_favorites->setHorizontalHeaderLabels(headers);
    ui->tableWidget_favorites->setColumnCount(headers.size());
    QHeaderView *header = ui->tableWidget_favorites->horizontalHeader();
    header->setSectionResizeMode(2, QHeaderView::Stretch);//出发地
    header->setSectionResizeMode(3, QHeaderView::Stretch);//目的地
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents); // 类型
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents); // 编号
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents); // 出发时间
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents); // 到达时间
    header->setSectionResizeMode(7, QHeaderView::ResizeToContents); // 公司
    header->setSectionResizeMode(6, QHeaderView::Interactive);
    ui->tableWidget_favorites->setColumnWidth(6, 85);
    header->setSectionResizeMode(8, QHeaderView::Fixed);
    ui->tableWidget_favorites->setColumnWidth(8, 70);
    ui->tableWidget_favorites->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_favorites->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_favorites->verticalHeader()->setVisible(false);
    header->setDefaultAlignment(Qt::AlignCenter);
}

void favorite_dialog::loadFavorites()
{
    if (currentUserID.isEmpty()) {
        QMessageBox::warning(this, "提示", "未登录用户，无法加载收藏！");
        return;
    }

    // 清空现有表格
    ui->tableWidget_favorites->setRowCount(0);

    // 查询收藏夹内容
    QSqlQuery query;
    query.prepare("SELECT t.flight_id, t.flight_number, t.departure_city, t.departure_airport, t.arrival_city, t.arrival_airport, "
                  "t.departure_time, t.arrival_time, t.price, t.airline_company "
                  "FROM favorites f JOIN flight_info t ON f.TicketID = t.flight_id "
                  "WHERE f.UserID = userID "
                  "ORDER BY t.departure_time DESC");
    query.bindValue(":userId", currentUserID.toInt());

    if (!query.exec()) {
        QMessageBox::critical(this, "错误", "查询收藏列表失败：" + query.lastError().text());
        return;
    }

    int row = 0;
    while (query.next()) {
        ui->tableWidget_favorites->insertRow(row);

        int ticketId = query.value(0).toInt();
        QString typeName = "航班";

        ui->tableWidget_favorites->setItem(row, 0, new QTableWidgetItem(typeName));
        ui->tableWidget_favorites->setItem(row, 1, new QTableWidgetItem(query.value(1).toString()));
        ui->tableWidget_favorites->setItem(row, 2, new QTableWidgetItem(query.value(2).toString()+query.value(3).toString()));
        ui->tableWidget_favorites->setItem(row, 3, new QTableWidgetItem(query.value(4).toString()+query.value(5).toString()));
        QDateTime depTime = query.value(6).toDateTime();
        ui->tableWidget_favorites->setItem(row, 4, new QTableWidgetItem(depTime.toString("yyyy-MM-dd hh:mm")));
        QDateTime arrTime = query.value(7).toDateTime();
        ui->tableWidget_favorites->setItem(row, 5, new QTableWidgetItem(arrTime.toString("yyyy-MM-dd hh:mm")));
        ui->tableWidget_favorites->setItem(row, 6, new QTableWidgetItem(QString::number(query.value(8).toDouble(), 'f', 2)));
        ui->tableWidget_favorites->setItem(row, 7, new QTableWidgetItem(query.value(9).toString()));
        for (int i = 0; i < 8; ++i) {
            if (ui->tableWidget_favorites->item(row, i)) {
                ui->tableWidget_favorites->item(row, i)->setTextAlignment(Qt::AlignCenter);
            }
        }
        // 添加移除按钮
        QPushButton *btnRemove = new QPushButton("移除");
        btnRemove->setProperty("ticketId", ticketId);
        connect(btnRemove, &QPushButton::clicked, this, &favorite_dialog::onRemoveFavorite);
        ui->tableWidget_favorites->setCellWidget(row,8, btnRemove);

        row++;
    }
}

void favorite_dialog::refreshFavoriteList()
{
    loadFavorites();
}

void favorite_dialog::on_btn_back_clicked()
{
    emit backRequested();
}

void favorite_dialog::on_btn_refresh_clicked()
{
    refreshFavoriteList();
}


void favorite_dialog::onRemoveFavorite()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int ticketId = btn->property("ticketId").toInt();

    if (currentUserID.isEmpty()) {
        QMessageBox::warning(this, "错误", "未登录用户！");
        return;
    }

    QSqlQuery removeQuery;
    removeQuery.prepare("DELETE FROM favorites WHERE UserID = :userId AND TicketID = :ticketId");
    removeQuery.bindValue(":userId", currentUserID.toInt());

    removeQuery.bindValue(":ticketId", ticketId);

    if (removeQuery.exec()) {
        QMessageBox::information(this, "成功", "已从收藏夹移除！");


        refreshFavoriteList(); // 刷新列表以移除该行
    } else {
        QMessageBox::critical(this, "错误", "移除失败：" + removeQuery.lastError().text());
    }
}


void favorite_dialog::on_searchBtn_clicked()
{
    if (currentUserID.isEmpty()) {
        return;
    }
    QString depCity = ui->lineEdit_dep->text().trimmed();     // 出发地
    QString arrCity = ui->lineEdit_arr->text().trimmed();     // 目的地
    QDate startDate = ui->dateEdit_begin->date();             // 开始日期
    QDate endDate = ui->dateEdit_end->date();                 // 结束日期

    QString sql = "SELECT t.flight_id, t.flight_number, t.departure_city, t.departure_airport, t.arrival_city, t.arrival_airport, "
                  "t.departure_time, t.arrival_time, t.price, t.airline_company "
                  "FROM favorites f JOIN flight_info t ON f.TicketID = t.flight_id "
                  "WHERE f.UserID = userId";

    if (!depCity.isEmpty()) {
        sql += " AND t.departure_city LIKE :depCity";
    }
    if (!arrCity.isEmpty()) {
        sql += " AND t.arrival_city LIKE :arrCity";
    }

    sql += " AND DATE(t.departure_time) >= :startDate";
    sql += " AND DATE(t.departure_time) <= :endDate";

    sql += " ORDER BY t.departure_time DESC"; // 排序
    QSqlQuery query;
    query.prepare(sql);
    query.bindValue(":userId", currentUserID.toInt());
    query.bindValue(":startDate", startDate);
    query.bindValue(":endDate", endDate);

    if (!depCity.isEmpty()) {
        query.bindValue(":depCity", "%" + depCity + "%"); // 模糊查询
    }
    if (!arrCity.isEmpty()) {
        query.bindValue(":arrCity", "%" + arrCity + "%"); // 模糊查询
    }

    if (!query.exec()) {
        QMessageBox::critical(this, "错误", "搜索失败：" + query.lastError().text());
        return;
    }

    ui->tableWidget_favorites->setRowCount(0); // 清空旧数据

    int row = 0;
    while (query.next()) {
        ui->tableWidget_favorites->insertRow(row);

        int ticketId = query.value(0).toInt();

        QString typeName = "航班";

        ui->tableWidget_favorites->setItem(row, 0, new QTableWidgetItem(typeName));

        ui->tableWidget_favorites->setItem(row, 1, new QTableWidgetItem(query.value(1).toString()));
        ui->tableWidget_favorites->setItem(row, 2, new QTableWidgetItem(query.value(2).toString()+query.value(3).toString()));
        ui->tableWidget_favorites->setItem(row, 3, new QTableWidgetItem(query.value(4).toString()+query.value(5).toString()));
        QDateTime depTime = query.value(6).toDateTime();
        ui->tableWidget_favorites->setItem(row, 4, new QTableWidgetItem(depTime.toString("yyyy-MM-dd hh:mm")));
        QDateTime arrTime = query.value(7).toDateTime();
        ui->tableWidget_favorites->setItem(row, 5, new QTableWidgetItem(arrTime.toString("yyyy-MM-dd hh:mm")));
        ui->tableWidget_favorites->setItem(row, 6, new QTableWidgetItem(QString::number(query.value(8).toDouble(), 'f', 2)));
        ui->tableWidget_favorites->setItem(row, 7, new QTableWidgetItem(query.value(9).toString()));
        //内容居中
        for (int i = 0; i < 8; ++i) {
            if (ui->tableWidget_favorites->item(row, i)) {
                ui->tableWidget_favorites->item(row, i)->setTextAlignment(Qt::AlignCenter);
            }
        }
        QPushButton *btnRemove = new QPushButton("移除");
        btnRemove->setProperty("ticketId", ticketId);
        connect(btnRemove, &QPushButton::clicked, this, &favorite_dialog::onRemoveFavorite);
        ui->tableWidget_favorites->setCellWidget(row, 8, btnRemove);
        row++;
    }

    if (row == 0) {
        QMessageBox::information(this, "提示", "未找到符合条件的收藏记录。");
    }
}
