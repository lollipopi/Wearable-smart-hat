/**********************************************************************************    
 *     工程名   ：基于Arduino单片机的可穿戴监护通讯设备
 *     硬件平台 ：Arduino Uno R3
 *     软件平台 ：Arduino IDE 1.6
 *     系统平台 ：Linux version 4.4.0-31-generic
 *     功能描述 ：* 监护功能项：1.温度实时测定；
 *                              2.湿度实时测定；
 *                              3.人体脉搏实时测定；                           
 *                              4.人体心率波动曲线；
 *                              5.传感器数据LCD实时显示；
 *                              
 *                * 监护报警项：1.一键紧急求救（电话&短信&报警音）
 *                              2.可燃气体监测语音报警；
 *                              3.火焰300°监测语音报警（火焰传感器数据待调试）；
 *                              4.体温过高语音报警；
 *                              
 *                * 通讯项    ：1.与紧急联系人紧急通话；
 *                              2.发送求救短信；
 *                              3.蓝牙（已实现配对功能，待开发）；
 *                              4.GPRS传感器数据实时上传&云端查看数据
 *                              （已完成测试，但未启用，详见代码）；
 *                              
 *                * 其他功能项：1.TTS语音（“让设备能够说话”）；              
 *                              2.电源输入电压显示；
 *                              3.超声波测距；
 *                              4.180°云台（舵机）.
 *                              
 *     库文件   ：详见 Github 
 *     修改日期 ：Tue, 04 Oct 2016 11:03:31 GMT
 *     版本     ：Final
 *     
 *     作者     ：开封高中 1812 李晨阳
 *     邮箱     ：molu@li.cm
 *     硬件连接 ：详见Github原理图
 *     Github   ：https://github.com/lollipopi/Wearable-smart-hat/
 *     更多信息 ：见申请书
**********************************************************************************/
                           
#include <U8glib.h>                   
#include <i2cmaster.h>
#include <DHT.h>
#include <Servo.h>

U8GLIB_ST7920_128X64_1X u8g(3, 9, 8);  //定义128*64LCD引脚
/*************  常量声明  **************/
#define DHTPIN 4    
#define DHTTYPE DHT11
DHT dht (DHTPIN, DHTTYPE);
Servo myservo;
/*************  变量声明  **************/
int inputPin = 6 ;  //定义超声波信号接收接口
int outputPin = 7 ; //定义超声波信号触发接口
int x, y;           //绘点坐标
int Buffer[128] ;   //缓存值储存数组
int tonepin = 10 ;  //蜂鸣器
int distance ;      //超声波
int pos = 0 ;       //用于存储舵机位置的变量
int val ;           //可燃气体
int k ;             //触控开关
int fire ;          //火焰传感器
int h ;             //心率传感器
float hum ;         //湿度
float tem ;         //温度
int blue =  1;      //限制蓝牙扫描次数
float celcius ;     //红外传感器

/**********************************************************************************    
 *     GPRS上传至Machtalk服务器功能待调试 详见申请书。
**********************************************************************************/
#define APIKEY "727d97154da4407fbbe62651aa9dd162"      //APIKEY
#define device_id "d5ebd1ba83f444f88a46a56840ae43c5"   //设备ID

/**********************************************************************************    
 *     函数名 ：sample
 *     描述   ：心率曲线绘制
 *     注意   ：
**********************************************************************************/
void sample( )
{
  //心率曲线
  for (x = 0; x < 128; x++)
    Buffer[x] = analogRead(A0);        //信号采样
  for (x = 0; x < 128; x++)
    Buffer[x] = 95 - (Buffer[x] >> 4); //计算纵坐标值
}

/**********************************************************************************    
 *     函数名 ：draw
 *     描述   ：128*64LCD显示
 *     注意   ：
**********************************************************************************/
void draw(void)
{
  u8g.setFont(u8g_font_tpss);                     //字体设置
  u8g.drawStr( 0, 10, "Wearable User Interface"); //交互
  u8g.drawStr( 0, 20, "Tem:");
  u8g.drawStr( 68, 20, "Hum:");
  u8g.drawStr( 0, 30, "Body T:");
  u8g.drawStr( 78, 30, "Dis:");
  u8g.drawStr( 114, 30, "cm");
  u8g.drawStr( 68, 30, "C");
  u8g.drawStr( 77, 60, "P:");
  u8g.drawStr( 107, 60, "bpm");
  u8g.drawStr( 2, 43, "ECG");

  //心率曲线
  for (x = 0; x < 127; x++)
  u8g.drawLine(x, Buffer[x], x, Buffer[x + 1]); //画相邻两点连线
  u8g.drawFrame(0, 32, 128, 32);                //画边框

  //传感器数据
  u8g.setPrintPos(28, 20);
  u8g.print(tem);
  u8g.setPrintPos(57, 20);
  u8g.print("C");

  u8g.setPrintPos(94, 20);
  u8g.print(hum);
  u8g.setPrintPos(120, 20);
  u8g.print("%");

  u8g.setPrintPos(96, 30);
  u8g.print(distance);

  u8g.setPrintPos(90, 60);
  u8g.print(h);

  u8g.setPrintPos(40, 30);
  u8g.print(celcius);
}

/**********************************************************************************    
 *     函数名 ：sensor
 *     描述   ：获取传感器数据
 *     注意   ：
**********************************************************************************/
void sensor()
{
  //温湿度
  hum = dht.readHumidity();
  tem = dht.readTemperature();

  //超声波
  digitalWrite(outputPin, LOW);        // 使发出发出超声波信号接口低电平2μs
  delayMicroseconds(2);
  digitalWrite(outputPin, HIGH);       // 使发出发出超声波信号接口高电平10μs
  delayMicroseconds(10);
  digitalWrite(outputPin, LOW);        // 保持发出超声波信号接口低电平
  distance = pulseIn(inputPin, HIGH);  // 读出脉冲时间
  distance = distance / 58;            // 将脉冲时间转化为距离（单位：厘米）
  
  //心率信号采样
  h = (analogRead(A0) / 10 );
  
  //红外温度
  int dev = 0x5A << 1;
  int data_low = 0;
  int data_high = 0;
  int pec = 0;

  i2c_start_wait(dev + I2C_WRITE);
  i2c_write(0x07);

  // read
  i2c_rep_start(dev + I2C_READ);
  data_low = i2c_readAck(); //Read 1 byte and then send ack
  data_high = i2c_readAck(); //Read 1 byte and then send ack
  pec = i2c_readNak();
  i2c_stop();

  //This converts high and low bytes together and processes temperature, MSB is a error bit and is ignored for temps
  double tempFactor = 0.02; // 0.02 degrees per LSB (measurement resolution of the MLX90614)
  double tempData = 0x0000; // zero out the data
  int frac; // data past the decimal point

  // This masks off the error bit of the high byte, then moves it left 8 bits and adds the low byte.
  tempData = (double)(((data_high & 0x007F) << 8) + data_low);
  tempData = (tempData * tempFactor) - 0.01;

  celcius = tempData - 273.15;
}

/**********************************************************************************    
 *     函数名 ：alert
 *     描述   ：报警响应
 *     注意   ：
**********************************************************************************/
void alert()
{
  //可燃气体报警
  val = analogRead(3);
  delay(100);

  if (val > 60)
  {
    tone(10, 80);
    Serial.println("AT+CTTS=1,6CE8610F51FA73B053EF71C36C144F53");  //"注意出现可燃气体" TTS
    delay(5000);
  }
  else
  {
    pinMode(tonepin, INPUT);
  }

  //触控开关报警（紧急求救）电话&短信（10086测试）
  k = digitalRead(11);
  if (k == HIGH)
  {
    tone(10, 20);
    Serial.println("AT+CTTS=1,5DF262E862537D27602575358BDDFF0C8BF77B495F85655163F4");  //"已拨打紧急电话，请等待救援" TTS
    Serial.print("AT+CMGF=1\r\n");//设置短信模式为TEXT
    delay(1000);
    Serial.print("AT+CMGS=\"17739205202\"\r\n"); //紧急联系人；短信接收方    
    delay(1000);
    Serial.print("SOS"); //求救短信内容
    delay(1000);
    Serial.write(0x1A);
    delay(2000);      
    Serial.print("AT+COLP=1\r\n");
    delay(1000);
    Serial.print("ATD10086;\r\n"); //紧急电话目标电话号码
    delay(10000);
  }
  else
  {
    pinMode(tonepin, INPUT);
  }

  //火焰报警
  fire = analogRead(2);
  delay(100);

  if (fire == 10000)   /*火焰传感器数据待调试*/
  {
    tone(10, 80);
    Serial.println("AT+CTTS=1,6CE8610F706B7130");  //"注意火焰" TTS
    delay(5000);
  }
  else
  {
    pinMode(tonepin, INPUT);
  }

  //高体温报警  （38°C报警）
  if ((int)celcius > 38)
  {
    tone(10, 80);
    Serial.println("AT+CTTS=1,4F536E298FC79AD862A58B66");  //"体温过高报警" TTS
    delay(5000);
  }
  else
  {
    pinMode(tonepin, INPUT);
  }
  
  //debug
  /*Serial.println(fire,DEC);
  Serial.println(val,DEC);*/
}

/**********************************************************************************    
 *     函数名 ：tower
 *     描述   ：控制舵机
 *     注意   ：
**********************************************************************************/
void tower()
{
  //舵机
  for (pos = 0; pos < 180; pos += 1)  // 从0度-180度
  {                                   // 步进角度1度
    myservo.write(pos);               // 输入对应的角度值，舵机会转到此位置
    delay(30);                        // 15ms后进入下一个位置
  }
  for (pos = 180; pos >= 1; pos -= 1) // 从180度-0度
  {
    myservo.write(pos);               // 输入对应的角度值，舵机会转到此位置
    delay(30);                        // 15ms后进入下一个位置
  }  
}

/**********************************************************************************    
 *     函数名 ：bluetooth
 *     描述   ：蓝牙相关功能实现
 *     注意   ：该项目仍处于调试状态，目前仅支持蓝牙配对。 功能待开发，详见申请书
**********************************************************************************/
void bluetooth ()
{
  if (blue <= 15)                   // 限制蓝牙扫描次数
  {
  Serial.println("AT+BTPAIR=1,1"); // 配对
  delay(50);
  Serial.println("AT+BTACPT=1");
  blue++;
  }
}

/**********************************************************************************    
 *     函数名：gprs
 *     描述  ：通过GPRS上传传感器数据至服务器，用户可从云端直接获取传感器数据。
 *     注意  ：*若启用该功能，则会导致循环过长，导致屏幕内容刷新周期过高。
               *预期解决方案：①增加单片机物理并行，执行两个主循环。
                              ②使用定时器，增加中断机制。
               *实验性功能，未正式启用。 详情见申请书。
               *测试数据上传链接：
     http://dev.machtalk.net/device/dataview/d5ebd1ba83f444f88a46a56840ae43c5
**********************************************************************************/
void gprs()
{
/******  体温  ******/
  Serial.println("AT+CIPSTART=\"TCP\",\"devapi.machtalk.net\",12086");
  delay(1500);
  Serial.println("AT+CIPSEND");
  delay(1000);
  String cmd;  
  cmd = "POST /v1.0/device/";
  cmd += String(device_id);
  cmd += "/1/1";
  cmd += "/datapoints/add";
  cmd += " HTTP/1.1\r\n";
  cmd += "Host: devapi.machtalk.net\r\n";
  cmd += "APIKey: ";
  cmd += APIKEY;
  cmd += "\r\n";
  cmd += "Accept: *";
  cmd += "/";
  cmd += "*\r\n";
  cmd += "Content-Length:22 ";
  cmd += "\r\n";
  cmd += "Content-Type: application/x-www-form-urlencoded\r\n";
  cmd += "Connection: close\r\n";
  cmd += "\r\n";
  cmd += "params={\"value\":";
  cmd += celcius;
  cmd += "}\r\n";
  Serial.print(cmd);
  delay(1000);
  Serial.write(26);
  delay(1500);
  Serial.println("AT+CIPCLOSE");
  delay(1000);

/******  心率  ******/
  Serial.println("AT+CIPSTART=\"TCP\",\"devapi.machtalk.net\",12086");
  delay(1500);
  Serial.println("AT+CIPSEND");
  delay(1000);
  cmd = "POST /v1.0/device/";
  cmd += String(device_id);
  cmd += "/2/1";
  cmd += "/datapoints/add";
  cmd += " HTTP/1.1\r\n";
  cmd += "Host: devapi.machtalk.net\r\n";
  cmd += "APIKey: ";
  cmd += APIKEY;
  cmd += "\r\n";
  cmd += "Accept: *";
  cmd += "/";
  cmd += "*\r\n";
  cmd += "Content-Length:22 ";
  cmd += "\r\n";
  cmd += "Content-Type: application/x-www-form-urlencoded\r\n";
  cmd += "Connection: close\r\n";
  cmd += "\r\n";
  cmd += "params={\"value\":";
  cmd += h;
  cmd += "}\r\n";
  Serial.print(cmd);
  delay(1000);
  Serial.write(26);
  delay(1500);
  Serial.println("AT+CIPCLOSE");
  delay(1000);
}

/**********************************************************************************    
 *     函数名 ：setup
 *     描述   ：初始化函数
 *     注意   ：
**********************************************************************************/
void setup()
{
  Serial.begin(9600);                   // 设置通讯的波特率为9600
  i2c_init();                            // 红外温度
  PORTC = (1 << PORTC4) | (1 << PORTC5); // enable pullups
  pinMode(inputPin, INPUT);              // 超声波
  pinMode(outputPin, OUTPUT);
  pinMode(11, INPUT);                    // 触控传感器
  myservo.attach(5);                     // 舵机控制信号引脚
  dht.begin();
  Serial.println("AT+CTTS=1,6B228FCE4F7F75288BE553EF7A7F623476D162A4901A8BAF8BBE5907FF0C8BE54EA754C14E3A79D16280521B65B059278D5B4F5C54C1");
                                         // "欢迎使用该可穿戴监护通讯设备，该产品为科技创新大赛作品"TTS
  Serial.println("AT+BTPOWER=1");       // 开启蓝牙电源
  delay(200);
  Serial.println("AT+BTHOST=Wearable"); // 修改蓝牙设备名
}

/**********************************************************************************    
 *     函数名 ：loop
 *     描述   ：主循环
 *     注意   ：GPRS项目仅为测试功能，未正式启用。详见申请书。
**********************************************************************************/
void loop(void)
{   
  alert ();     // 报警
  tower ();     // 舵机
  //gprs ();    // GPRS数据上传  /* 实验性功能 */
  sensor ();    // 传感器采集数据
  bluetooth (); // 蓝牙模块
  sample();     // 绘图采样
  
  u8g.firstPage();
  do {
    draw();
  } while ( u8g.nextPage() );
}
