#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define RST_PIN         D2          // Cấu hình chân RST của RFID
#define SS_PIN          D4          // Cấu hình chân SS của RFID
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

// Định nghĩa LED
const int ledPin1 = D8; // sáng nếu đúng PASSWORD
const int ledPin2 = D3; // sáng nếu sai PASSWORD



ESP8266WebServer server(80); //CỔNG HTTP

const char* host = "script.google.com";
const int httpsPort = 443;

String GAS_ID = "AKfycbwlhCgI7pMtcFrOJctvzNJwKb3hEuGTVyX2ylSSHpe5f3RuZySODrrzWB7YzZ2Duqi4";  // Thay GAS_ID bằng ID của script của bạn

WiFiClientSecure client;

//ANALOG VALUE FOR READ KEY
//int thresholds[16] = {900, 520, 350, 270, 
//                      780, 475, 330, 260, 
//                      675, 440, 310, 245, 
//                      600, 400, 290, 235};
int thresholds[16] = {245, 350, 502, 898, 
                      235, 330, 465, 805, 
                      225, 315, 435, 703, 
                      215, 295, 400, 614};
char key[16] = {'1','2','3','A',
                '4','5','6','B',
                '7','8','9','C',
                '*','0','#','D'};

unsigned long lastDebounceTime = 0; // Thời gian debounce
const unsigned long debounceDelay = 200; // Thời gian chờ debounce

// Tài khoản hợp lệ
String validAccounts[] = {"224", "468", "789"};  // Các tài khoản hợp lệ
bool isAccountValid = false;
String account="";// tài khoản nhập từ keypad
String web_account = ""; 

String password = "555"; // Mật khẩu mặc định
String input_password = ""; // Mật khẩu nhập từ keypad
String web_notification = "";  // Chuỗi thông báo trên Webserver 
String web_input_password = ""; // Mật khẩu nhập từ web

void setup() {
  Serial.begin(115200);
  Led_Init();
  Oled_Init();
  RFID_Init();
  WiFi_Init();
  WebServer_Init();
}

void loop() {
  log_in();
  server.handleClient(); // Xử lý yêu cầu từ WebServer
}

// Khởi tạo LED
void Led_Init() {
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);   
}
// Khởi tạo màn hình OLED
void Oled_Init() { 
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); 
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Nhap tai khoan:");
  display.display();
}
//Khởi tạo RFID
void RFID_Init(){
  SPI.begin();                       
  mfrc522.PCD_Init();
  //Serial.println("Put your card on the reader.");
}
// Khởi tạo kết nối Wi-Fi
void WiFi_Init() {
  WiFi.mode(WIFI_STA);
  WiFi.begin("Searching..", "tumotdenmuoi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setInsecure();  // Bỏ qua chứng chỉ SSL (chỉ sử dụng cho mục đích thử nghiệm, không an toàn cho môi trường thực tế)
}

//Gửi data lên google sheet
void sendata(String state, String account){
  Serial.print("Connecting to ");
  Serial.println(host);

  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    return;
  }

  // Mã hóa chuỗi status trước khi gửi
  String encodedStatus = urlEncode(state);
  String encodedAccount  = urlEncode(account);  

  // Tạo URL với tham số status
  String url = "/macros/s/" + GAS_ID + "/exec?state=" + encodedStatus + "&account=" + encodedAccount;
  Serial.println(url);

  // Gửi yêu cầu GET đến Google Apps Script
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");
  Serial.println("OK");
}

// Hàm để mã hóa chuỗi thành URL encoding
String urlEncode(String str) {
  String encoded = "";
  char c;
  for (unsigned int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encoded += "%20";  // Thay thế dấu cách thành %20
    } else if (c == '&') {
      encoded += "%26";  // Thay thế dấu & thành %26
    } else {
      encoded += c;
    }
  }
  return encoded;
}

//CORRECT PASSWORD
void Correct(){
  display.clearDisplay(); // Xóa màn hình trước khi hiển thị
  display.setCursor(0, 0);
  display.println("DANG NHAP THANH CONG");
  digitalWrite(ledPin1, HIGH);  // LED 1 ON KHI ĐÚNG PASS
  digitalWrite(ledPin2, LOW);   // LED 2 OFF KHI ĐÚNG PASS
  sendata("DANG NHAP THANH CONG",account);  // Gửi dữ liệu lên Google Sheets
}
//WRONG PASS
void Wrong(){
  display.clearDisplay(); // Xóa màn hình trước khi hiển thị
  display.setCursor(0, 0);
  display.println("SAI MAT KHAU");
  digitalWrite(ledPin1, LOW);   // LED 1 OFF KHI SAI PASS
  digitalWrite(ledPin2, HIGH);  // LED 2 ON KHI ĐÚNG PASS
  sendata("SAI MAT KHAU",account);  // Gửi dữ liệu lên Google Sheets
}
//RESET
void Reset(){
  web_input_password = "";  // Clear input password
  input_password = "";
  account="";
  web_account = "";
  isAccountValid = false; // Đặt lại trạng thái tài khoản
  digitalWrite(ledPin1, LOW);  // off 2LED
  digitalWrite(ledPin2, LOW);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Reset!!!");
  display.display();
  delay(1000);  // Show reset message for 1 second
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Nhap tai khoan:");
  display.display();
}
// Hàm đọc phím từ keypad
char readkey() {
  int val = analogRead(A0);  // Đọc giá trị analog hiện tại
  Serial.println(val);
  unsigned long currentTime = millis(); // Lấy thời gian hiện tại
  
  // Kiểm tra nếu đã qua thời gian debounce
  if ((currentTime - lastDebounceTime) > debounceDelay) {
    // Kiểm tra phím nhấn bằng cách so sánh với ngưỡng trong mảng thresholds
    for (int i = 0; i < 16; i++) {
      if (abs(val - thresholds[i]) < 8) {
        Serial.println(key[i]);  // In ra phím đã nhấn
        lastDebounceTime = currentTime;  // Cập nhật thời gian debounce
        return key[i];  // Trả về ký tự đã nhấn
      }
    }
  }
  delay(1000);
  return '\0';  // Không có phím nào được nhấn
}

// Hàm nhập tài khoản từ keypad
void enter_account() {
  char acc = readkey(); // Đọc phím từ keypad

  if (acc != '\0') { // Kiểm tra nếu có phím được nhấn
    Serial.print("Key pressed: ");
    Serial.println(acc);

    if (acc == 'A') { // Xác nhận tài khoản khi nhấn phím 'A'
      display.clearDisplay();
      display.setCursor(0, 0);

      // Kiểm tra tài khoản hợp lệ
      isAccountValid = false; // Mặc định là không hợp lệ
      for (int i = 0; i < 3; i++) {
        if (account == validAccounts[i]) {
          isAccountValid = true;
          break; // Thoát khỏi vòng lặp nếu tài khoản hợp lệ
        }
      }

      if (isAccountValid) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Nhap mat khau or quet the:");
      } else {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Sai tai khoan!");
      }

      display.display();
 //     account = ""; // Xóa tài khoản nhập sau khi xác nhận
    } else if (acc == 'B') { // Reset tài khoản khi nhấn phím 'B'
      Reset();
    } else { // Thêm ký tự vào tài khoản
      account += acc;
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Tai khoan: ");
      display.print(account);
      display.display();
    }
  }
  RFID_Init();
}


//PASSWORD KEYPAD
void pass_key(){
  char pass = readkey();  // Read KEYPAD
  
  // Handle keypad input
  if (pass != '\0') {  // KIỂM TRA NÚT NHẤN
    Serial.print("Key pressed: ");
    Serial.println(pass);

    if (pass == 'A') {  // Button "A" to confirm password
      display.clearDisplay();
      display.setCursor(0, 0);
      if (input_password == password) {
        Correct();
     //   sendata("DANG NHAP THANH CONG",account);  // Gửi dữ liệu lên Google Sheets
      } 
      else {
        Wrong();
       // sendata("SAI MAT KHAU",account);  // Gửi dữ liệu lên Google Sheets
      }
      display.display();
    } else if (pass == 'B') {  // Button "B" to reset password input
      Reset();
    } else {
      // Add key to the input password
      input_password += pass;
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Mat khau: ");
      display.print(input_password);
      display.display();
    }
    delay(1000);
    Serial.println(input_password);
    RFID_Init();
  }
}
//PASSWORD RFID
void pass_rfid(){
  // Handle RFID card reading
 if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
  Serial.println("Card detected.");
  String content = "";

  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  content.toUpperCase();
  Serial.println(); // New line after UID
  Serial.println("Message:");
  Serial.println("UID: " + content);  // In UID của card để debugging

     // Compare UID and control LEDs
     if (content == "03F98C13") {
         Correct();
        // sendata("DANG NHAP THANH CONG",account);  // Gửi dữ liệu lên Google Sheets
     } 
    else {
        Wrong();
       // sendata("SAI MAT KHAU",account);  // Gửi dữ liệu lên Google Sheets
    }
} else {
  Serial.println("No card present or error reading card.");
  delay(1000);
  }
}
//nhap mat khau
void log_in(){
  if (!isAccountValid) {
    enter_account();
  } else {
    pass_rfid();
    pass_key();
    
  }
}

// Khởi tạo WebServer
void WebServer_Init() {
  server.on("/", []() {
    server.send(200, "text/html", send_html());
  });

  server.on("/submit", []() {
    String key = server.arg("key");       // Lấy phím từ form
    String accountInput = server.arg("account"); // Lấy tài khoản nhập từ form
    char webKey = key[0];                // Chuyển ký tự đầu tiên của phím thành char

    if (accountInput != "") {
      account = accountInput;        // Lưu tài khoản nhập vào
    }

    // Kiểm tra tài khoản hợp lệ
    if (!isAccountValid) { // Nếu tài khoản chưa hợp lệ, kiểm tra
      isAccountValid = false; // Mặc định không hợp lệ
      for (int i = 0; i < 3; i++) {
        if (account == validAccounts[i]) {
          isAccountValid = true;
          break;
        }
      }

      if (isAccountValid) {
        web_notification = "Nhap mat khau:";
        web_input_password = ""; // Reset mật khẩu để nhập lại
      } else {
        web_notification = "Sai tai khoan!";
      }

      // Cập nhật giao diện và gửi lại HTML
      server.send(200, "text/html", send_html());
      return; // Kết thúc ở đây nếu kiểm tra tài khoản
    }

    // Xử lý nhập mật khẩu
    if (webKey == 'A') {  // Xác nhận mật khẩu
      if (web_input_password == password) {
        Correct();
        web_notification = "DANG NHAP THANH CONG";
        //sendata("DANG NHAP THANH CONG",account);  // Gửi dữ liệu lên Google Sheets
      } else {
        Wrong();
        web_notification = "SAI MAT KHAU";
        //sendata("SAI MAT KHAU",account);  // Gửi dữ liệu lên Google Sheets
      }
      web_input_password = ""; // Xóa mật khẩu sau khi kiểm tra
    } else if (webKey == 'B') { // Reset mật khẩu
      Reset();
      web_notification = "Password Reset";
    } else { // Thêm ký tự vào mật khẩu
      web_input_password += webKey;
      web_notification = ""; // Xóa thông báo khi nhập
    }

    server.send(200, "text/html", send_html()); // Cập nhật giao diện HTML
  });

  server.begin();
  Serial.println("HTTP server started");
}

// Hàm HTML để hiển thị giao diện web với nhập tài khoản và mật khẩu
String send_html() {
  String html = "<html><body>";
  html += "<h1>Enter Account and Password</h1>";
  html += "<form action='/submit' method='POST'>";

  // Ô nhập tài khoản
  html += "<label for='account'>Account:</label>";
  html += "<input type='text' id='account' name='account' value='" + account + "' required><br><br>";

  // Tạo keypad 4x4
  html += "<table>";
  html += "<tr><td><button name='key' value='1'>1</button></td><td><button name='key' value='2'>2</button></td><td><button name='key' value='3'>3</button></td><td><button name='key' value='A'>A</button></td></tr>";
  html += "<tr><td><button name='key' value='4'>4</button></td><td><button name='key' value='5'>5</button></td><td><button name='key' value='6'>6</button></td><td><button name='key' value='B'>B</button></td></tr>";
  html += "<tr><td><button name='key' value='7'>7</button></td><td><button name='key' value='8'>8</button></td><td><button name='key' value='9'>9</button></td><td><button name='key' value='C'>C</button></td></tr>";
  html += "<tr><td><button name='key' value='*'>*</button></td><td><button name='key' value='0'>0</button></td><td><button name='key' value='#'>#</button></td><td><button name='key' value='D'>D</button></td></tr>";
  html += "</table>";

  // Hiển thị thông tin
  html += "<p>Account: " + account + "</p>";
  html += "<p>Password: " + web_input_password + "</p>";
  html += "<p><b>" + web_notification + "</b></p>";
  html += "</form></body></html>";
  
  return html;
}
