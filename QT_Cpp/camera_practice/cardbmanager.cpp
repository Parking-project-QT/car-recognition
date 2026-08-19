#include "cardbmanager.h"


CarDBManager::CarDBManager()
{

}

bool CarDBManager::connectDB()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("parking.db");

    if(!db.open())
    {
        qDebug() << "DB 연결 실패";
        qDebug() << db.lastError().text();
        return false;
    }

    qDebug() << "DB 연결 성공";

    QSqlQuery query;

    query.exec("CREATE TABLE IF NOT EXISTS parking ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "car_number TEXT UNIQUE, "
        "in_time TEXT,"
        "is_deleted BOOLEAN DEFAULT 0 CHECK(is_deleted IN (0,1))"
        ")");

    return true;
}

bool CarDBManager::is_car_parked(const QString &carnum)
{
    QSqlQuery query;

    //데이터 베이스에 전달한 sql 퀴리 문장 미리 컴파일하고 준비
    query.prepare(
        "Select * FROM parking "
        "WHERE car_number = :car_number "
        "AND is_deleted = 0"
    );

    query.bindValue(":car_number",carnum); //sql 퀴리를 안전하게 값을 바인딩 함수

    if(!query.exec())
    {
        qDebug() << "차량 조회 실패";
        qDebug() << query.lastError().text();

        return false;
    }
    if(query.next()) //다음 데이터 요청
    {
        return true;
    }

    return false;
}
bool CarDBManager::car_in(const QString &carnum)
{
    QSqlQuery query;

    query.prepare(
        "INSERT INTO parking "
        "(car_number, in_time, is_deleted) "
        "VALUES "
        "(:car_number, datetime('now','localtime'), 0) "
        "ON CONFLICT (car_number) " //바인딩 변수가 아니라서
        "DO UPDATE SET "
        "in_time = EXCLUDED.in_time, "
        "is_deleted = EXCLUDED.is_deleted; "
    );

    query.bindValue(":car_number",carnum);

    if(!query.exec())
    {
        qDebug() << "입차 등록 실패";
        qDebug() << query.lastError().text();

        return false;
    }

    qDebug() << "입차 환료 : " << carnum;
    return true;
}
bool CarDBManager::car_out(const QString &carnum)
{
    QSqlQuery query;

    query.prepare(
        "UPDATE parking "
        "SET "
        "is_deleted = 1 "
        "WHERE car_number = :car_number "
        "AND is_deleted = 0; "
    );

    query.bindValue(":car_number",carnum);

    if(!query.exec())
    {
        qDebug() << "출차 실패";
        qDebug() << query.lastError().text();

        return false;
    }

    qDebug() << "출차 환료 : " << carnum;
    return true;

}
