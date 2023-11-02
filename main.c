#Bui Chi Kien

#include <wiringPi.h>
#include <mysql.h>
#include <wiringPiI2C.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>  //int16_t 
#include <math.h> 
#define temp_address    65
#define Sample_rate     25
#define Config          26
#define Gyro_config     27
#define Acc_config      28
#define Interrupt       56
#define PWR_managment   107
#define channel 0

#define Acc_adress 59

#define red 36
#define blue 38
#define buzzer 32
#define bt1 31
#define bt2 33
#define bt3 35
#define bt4 37
int dieukhien;  // luu so cot table vao bien num_column
int dem, count;
int mpu;
float van_toc, Acc_prev, van_toc_prev, Acc_average;
float van_toc_hien_tai;
float alpha_x , alpha_y;
struct mpu_value Acc;
void Init_6050(void)
{
    // register: 25->28, 56, 107
    // sample_rate 500Hz: 2ms do xong 1 mau
    wiringPiI2CWriteReg8(mpu, Sample_rate, 1);
    // Khong su dung nguon xung ngoai, tat DLPF
    wiringPiI2CWriteReg8(mpu, Config, 5);
    // gyro FS: +- 500 o/s
    wiringPiI2CWriteReg8(mpu, Gyro_config, 8);
    // acc FS: +- 8g
    wiringPiI2CWriteReg8(mpu, Acc_config, 0x10);
    // mo interrupt cua data ready: do xong 1 mau thi thay doi chan inte cua MPU6050
    wiringPiI2CWriteReg8(mpu, Interrupt, 1);
    // chon nguon xong cho Gyro X  
    wiringPiI2CWriteReg8(mpu, PWR_managment, 1);
}
int16_t read2res(uint8_t address)
{
    int16_t data;
    data = wiringPiI2CReadReg8(mpu, address) << 8;
    data = data | wiringPiI2CReadReg8(mpu, address+1);
    return data;
}
struct mpu_value
{
    float x; 
    float y; 
    float z;
};
struct mpu_value read_sensor(uint8_t address, float sensitivity)
{    
    struct  mpu_value data;
    data.x = (float)read2res(address)/sensitivity*9.8;
    data.y = (float)read2res(address+2)/sensitivity*9.8;
    data.z = (float)read2res(address+4)/sensitivity*9.8;
    return data;
};
void mpu_int(void)
{
    Acc = read_sensor(Acc_adress, 4096.0); //don vi la "g"
    Acc_average = sqrt(Acc.x*Acc.x + Acc.y*Acc.y + Acc.z*Acc.z); 

    alpha_x = atan2f(Acc.y,Acc.z)*180/3.14;
    alpha_y = atan2f(Acc.x,Acc.z)*180/3.14;
    printf("Goc alpha_x = %.1f do, alpha_y = %.1f do \n",alpha_x,alpha_y);
    printf("gia toc truc x = %.2f m/^2,  y= %.2f m/s^2,   z= %.2f m/s^2\n",Acc.x,Acc.y,Acc.z);
    wiringPiI2CReadReg8(mpu, 0x58);   //xoa co ngat
}
void velocity()
{   
    van_toc_prev = 0;
    van_toc = 0.5*0.1*(Acc_prev + Acc_average) + van_toc_prev;
    van_toc_prev = van_toc;
    Acc_prev = Acc_average;
    van_toc_hien_tai = fabs(van_toc-1.03)*100;
    printf("van toc hien tai V = %.1f cm/s   gia toc trung binh Acc_avg = %.1f  dem  =%d count=%d\n\n",van_toc_hien_tai, Acc_average,dem,count);
}
void check_fall()
{
    if(Acc_average > 20 )   //neu nga  buzzer keu
    {
        digitalWrite(buzzer,1);
        digitalWrite(red,1);
        digitalWrite(blue,0);
        count = 1;
        dieukhien = 0;
    }

}

void tat_buzzer(void)
{
        digitalWrite(buzzer,0);
        digitalWrite(red,0);
        digitalWrite(blue,1);
        count = 0;
}
void support(void)
{
    dem++;
    if(dem==2) 
    {
    dem=0; 
    }
}
void canhbao(void)
{
    digitalWrite(blue,0);
        for(int i=0; i<5; i++)
        {
            digitalWrite(buzzer,1);
            digitalWrite(red,1);
            delay(300);
            digitalWrite(buzzer,0);
            digitalWrite(red,0);
            delay(300);
        }
}
void setup(void)
{
    
    // setup();
    mpu = wiringPiI2CSetup(0x68);
    //setup giao tiep i2c
    wiringPiSetupPhys();
    Init_6050();
    pinMode(red, OUTPUT);
    pinMode(blue, OUTPUT);
    pinMode(bt1, INPUT);
    pinMode(bt2, INPUT);
    pinMode(buzzer,OUTPUT);
    digitalWrite(buzzer,0);
    digitalWrite(red,0);
    digitalWrite(blue,1);
    wiringPiISR(bt1,INT_EDGE_RISING,&tat_buzzer);
    wiringPiISR(bt2,INT_EDGE_RISING,&support);
}

int main(void)
{ 
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char *server = "localhost";
    char *user = "bao_bui";
    char *password = "1"; 
    char *database = "cuoiki1";
    conn = mysql_init(NULL);
    mysql_real_connect(conn,server,user,password,database,0,NULL,0); 
 
    mysql_query(conn,"select * from nutnhan1");
    res = mysql_store_result(conn); // luu data vao bien res
    int num_column = mysql_num_fields(res);  // luu so cot table vao bien num_column
    row = mysql_fetch_row(res);

    setup();

    while (1)
    {   
        mysql_query(conn,"select * from nutnhan1");
        res = mysql_store_result(conn); // luu data vao bien res
        int num_column = mysql_num_fields(res);  // luu so cot table vao bien num_column
        row = mysql_fetch_row(res);
        if(count==0)
        {
            if(atoi(row[1]) == 0) // Yes
            {
                canhbao();
            }
            if(atoi(row[1]) == 1) // Yes
            {
                digitalWrite(red,0);
                digitalWrite(blue,1);
            }
        }
        char sql[200];
        sprintf(sql,"insert into nguoigia2(giatri , giatoc , nutnhan,count) values (%.1f,%.2f,%d,%d)",van_toc_hien_tai,Acc_average,dem,count);
        // send SQL query 
        mysql_query(conn, sql);
        mpu_int();
        velocity();
        check_fall();   
        delay(100);

    }
    mysql_close(conn);
    return 0;
}
