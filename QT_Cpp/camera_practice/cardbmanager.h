#ifndef CARDBMANAGER_H
#define CARDBMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>


class CarDBManager
{
public:
    CarDBManager();

    bool connectDB();
    bool is_car_parked(const QString &carnum);
    bool car_in(const QString &carnum);
    bool car_out(const QString &carnum);

private:
    QSqlDatabase db; //선언

};

#endif // CARDBMANAGER_H
