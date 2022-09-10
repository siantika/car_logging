/* main code */
// Pastikan untuk memasukkan kabel usb to UART dan INGAT UNTUK TIDAK MEMASUKKAN PWR IN dari adaptor !!! (hanya UART saja)
// untuk melihat pesan debug, silakan pergi ke tab header.h dan uncomment #define DEBUG pada baris 13.
// untuk deploying, debug mode mohon di-comment kembali!.
// Terima kasih

// library
#include "header.h"
#include "IoPin.h"
#include "USBPrinter.h"
#include "ESC_POS_Printer.h"
#include <avr/wdt.h>
#include <ezButton.h>
#include <Ethernet.h>
#include <HttpClient.h>

/* PrinterOper class*/
// harus di taruh di atas properties kelas. default
class PrinterOper : public USBPrinterAsyncOper
{
  public:
    uint8_t OnInit(USBPrinter *pPrinter);
};

uint8_t PrinterOper::OnInit(USBPrinter *pPrinter)
{
  DPRINTLN(F("USB Printer OnInit"));
  DPRINT(F("Bidirectional="));
  DPRINTLN(pPrinter->isBidirectional());
  return 0;
}
/* ************************* PrinterOper class ends here ************************ */
/* Printer Properties */
USB myusb;
PrinterOper AsyncOper;
USBPrinter  uprinter(&myusb, &AsyncOper);
ESC_POS_Printer printer(&uprinter);
/* *********************************************************** */

/* Ethernet Properties */
IPAddress ip(192, 168, 1, 177); // IP alat ini.
EthernetClient client;
HttpClient http(client);
const byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //Setting MAC Address Alat
const char server[] = "192.168.1.88"; // HOST Server-nya
const char path_base_data[] = "/pintum_b.php"; // alamat untuk ambil data layanan
const char path_last_id[] = "/counter_b.php"; // alamat untuk ambil data barcode id terakhir dan POST data layanan ke server lokal
/* *********************************************************** */

/* Button's Properties */
ezButton button_1(PIN_BUTTON_1);
ezButton button_2(PIN_BUTTON_2);
ezButton button_3(PIN_BUTTON_3);
ezButton button_4(PIN_BUTTON_4);
/* *********************************************************** */

/* Variables */
uint8_t state;
String* ptr_selected_jenis_layanan;
String* ptr_selected_harga;
String base_data_from_database[TOTAL_INDEX]; // data layanan dan harga dari database
String kode_jenis_layanan = "";
String jenis_layanan_1 = "";
String jenis_layanan_2 = "";
String jenis_layanan_3 = "";
String jenis_layanan_4 = "";
String harga_jenis_layanan_1 = "";
String harga_jenis_layanan_2 = "";
String harga_jenis_layanan_3 = "";
String harga_jenis_layanan_4 = "";
unsigned long id_barcode;
char raw_data[BARCODE_LENGTH]; // kepemilikan fungsi convertToAscii, HARUS dideklarasikan secara GLOBAL. ini untuk barcode
char converted_to_ascii[BARCODE_LENGTH]; // ini untuk barcode
String str_barcode="";

// pendeklarasian nama fungsi di awal --> sesuai dengan standar bahasa C
void sendDataToServer(String* jenis_layanan, String* harga);
void (*ptrSendDataToServer)(String* jenis_layanan, String* harga) = sendDataToServer; // pointer function
void openPortal();
void readButtons();
void clearValue();
void parsingData(String arr_base_data[]);
void printerEpson(unsigned long id_barcode, String* jenis_layanan, String* harga);
void autoCutter();
void printBarcode(char data_id[], int len_id);
void convertToAscii(unsigned long number, int len_number);
void statusLedOff();
void statusLedOn();

/* SETUP */
void setup() {
  // hidupakan wdt
  //wdt_enable(WDTO_8S);

  Serial.begin(9600);
  pinMode(PIN_PORTAL_TRIGGER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_PORTAL_TRIGGER, HIGH); //portal  active low
  DPRINTLN("MULAI");

  // inisiasi printer
  while (!Serial && millis() < 3000) delay(1);

  if (myusb.Init()) {
    DPRINTLN(F("USB host failed to initialize"));
    while (1) delay(1);
  } else {
    DPRINTLN("USB HOST berhasil");
  }
  statusLedOff();

  // Inisiasi ethernet W5500
  Ethernet.init(CS_ETHERNET);
  // Static IP
  Ethernet.begin(mac, ip); // ISI IP
  Serial.println(Ethernet.localIP());
  // buat koneksi ke database sampai berhasil
  //  while (http.get(server, path_base_data) != 0)
  //  {
  //    Serial.println("Data request");
  //    int error = http.get(server, path_base_data); // get data from database
  //    Serial.println(error);
  ////    delay(5000);
  //  }

  // data jenis layanan tombol
  int error = http.get(server, path_base_data);
  DPRINTLN(error);
  // Ambil hanya body html-nya saja
  error =  http.skipResponseHeaders();
  DPRINTLN(error);

  // parsing data dari database
  parsingData(base_data_from_database);
  //  int i = 0;
  //  while (http.available() || http.connected())
  //  {
  //    char c = http.read();
  //    if (c != '#') {
  //      base_data_from_database[i] += c;
  //    } else if (c == '#') {
  //      i++;
  //    }
  //    DPRINT(c);
  //  }

  // simpan data yang telah di-parsing pada variabel
  jenis_layanan_1 = base_data_from_database[0];
  jenis_layanan_2 = base_data_from_database[2];
  jenis_layanan_3 = base_data_from_database[4];
  jenis_layanan_4 = base_data_from_database[6];
  DPRINT("Debug data jenis layanan: ");
  DPRINTLN(jenis_layanan_1);
  harga_jenis_layanan_1 = base_data_from_database[1];
  harga_jenis_layanan_2 = base_data_from_database[3];
  harga_jenis_layanan_3 = base_data_from_database[5];
  harga_jenis_layanan_4 = base_data_from_database[7];

  http.stop(); // HARUS DIISI
  delay(1000); // HARUS DIISI



  // mendapatkan last id barcode
  error = http.get(server, path_last_id);
  DPRINTLN(error);
  error = http.skipResponseHeaders();
  DPRINTLN(error);

  id_barcode = http.readString().toInt();

  // debug

  if (id_barcode == 0)
    id_barcode = 1; // jika tidak ada nilai id barcode di database akhir

  // LED hidup karena data berhasil diambil
  statusLedOn();

  DPRINT(F("Kode terakhir: "));
  DPRINTLN(id_barcode);

  button_1.setDebounceTime(DEBOUNCE_TIME);
  button_2.setDebounceTime(DEBOUNCE_TIME);
  button_3.setDebounceTime(DEBOUNCE_TIME);
  button_4.setDebounceTime(DEBOUNCE_TIME);

  // pindah ke state langsung = 1;
  state = 1;
}

/* DRIVER CODE: LOOPING */
void loop() {
  /* kode - kode ini harus selalu dieksekusi setiap alur/case yang dipanggil */
  myusb.Task();
  button_1.loop();
  button_2.loop();
  button_3.loop();
  button_4.loop();
  /* ****************************** */

  /* Menggunakan Desain: Finite State Machine */
  switch (state)
  {
    case 1:
      {
        readButtons(); // event: baca tombol-tombol
        // state ada di dalam function readButtons()karena state akan berpindah jika tombol ditekan, jika tidak ada ditekan, maka state tetap di state 1 (baca tombol)
        break;
      }
    case 2:
      {
        convertToAscii(id_barcode, BARCODE_LENGTH);
        // filter the barcode
        for (int i = 0; i < BARCODE_LENGTH; i++)
        {
            str_barcode += converted_to_ascii[i];
        }
        DPRINT(str_barcode);
        DPRINTLN();
        state = 3;
        break;
      }
    case 3:
      {
        sendDataToServer(ptr_selected_jenis_layanan, ptr_selected_harga);
        break;
      }
    case 4:
      {
        printerEpson(id_barcode, ptr_selected_jenis_layanan, ptr_selected_harga);
        state = 5;

        // debug BARCODE
        /******************************/
        DPRINTLN("Print Barcode");
        for (int i = 0; i < BARCODE_LENGTH; i++)
          DPRINT(converted_to_ascii[i]);
        DPRINTLN();
        /******************************/
        break;
      }
    case 5:
      {
        openPortal();
        state = 6;
        break;
      }
    case 6:
      {
        clearValue();
        state = 1;
        break;
      }
    case 7:
      {
        // server mati atau ethernet bermasalah
        statusLedOff();
        state = 1;
        break;
      }
  }

  // reset wdt
  wdt_reset();
}

// Fungsi-fungsi
void sendDataToServer(String* jenis_layanan, String* harga)
{
  if (client.connect(server, 80)) {
    DPRINTLN("connected");
    statusLedOn(); // hidupkan status LED --> koneksi aman

    // Make a HTTP request:
    client.print(F("GET /counter_b.php?jenis_layanan="));     //alamat server post data nya
    client.print(*jenis_layanan);
    client.print(F("&id_barcode="));// add 0 --> menyesuaikan dengan scanner pada pintu keluar
    client.print(str_barcode); // id barcode dikonversi menjadi ASCII string. // CEK DULU
    client.print(F("&kode_pintu="));
    client.print(kode_jenis_layanan);
    client.print(F(" "));      //SPACE BEFORE HTTP/1.1
    client.print(F("HTTP/1.1"));
    client.println();
    client.println(F("Host: 192.168.1.88"));  // alamat server-nya
    client.println(F("Connection: close"));
    client.println();

    state = 4; // pindah ke state 4

    // debug
    DPRINT(F("ID "));
    DPRINTLN(id_barcode);
    DPRINT(F("jenis Layanan: "));
    DPRINTLN(*jenis_layanan);
    DPRINT(F("Harga: "));
    DPRINTLN(*harga);
    DPRINTLN(F("--------------------------------------"));

  } else {
    // if you didn't get a connection to the server:
    state = 7;
    DPRINTLN("connection failed");
  }
}

// event: baca tombol-tombol
void readButtons()
{
  if (button_1.isReleased())
  {
    ptr_selected_jenis_layanan = &jenis_layanan_1 ;
    ptr_selected_harga = &harga_jenis_layanan_1 ;
    kode_jenis_layanan = "AA";
    id_barcode ++; // increment for barcode
    state = 2; // pindah ke state 2
  }

  else if (button_2.isReleased())
  {
    ptr_selected_jenis_layanan = &jenis_layanan_2;
    ptr_selected_harga = &harga_jenis_layanan_2 ;
    kode_jenis_layanan = "BB";
    id_barcode ++; // increment for barcode
    state = 2;
  }

  else if (button_3.isReleased())
  {
    ptr_selected_jenis_layanan = &jenis_layanan_3;
    ptr_selected_harga = &harga_jenis_layanan_3;
    kode_jenis_layanan = "CC";
    id_barcode ++; // increment for barcode
    state = 2;
  }

  else if (button_4.isReleased())
  {
    ptr_selected_jenis_layanan = &jenis_layanan_4;
    ptr_selected_harga = &harga_jenis_layanan_4;
    kode_jenis_layanan = "DD";
    id_barcode ++; // increment for barcode
    state = 2;
  }
}

/* Bersihkan data sebelum memulai mebaca masukkan dari user (tombol ditekan) */
void clearValue()
{
  ptr_selected_jenis_layanan = NULL;
  ptr_selected_harga = NULL;
  kode_jenis_layanan = "";
  str_barcode = "";
  memset(raw_data, 0, sizeof(raw_data));
  memset(converted_to_ascii, 0, sizeof(converted_to_ascii));
}

/* buka palang */
void openPortal()
{
  digitalWrite(PIN_PORTAL_TRIGGER, LOW);
  delay(DELAY_TIME_FOR_PORTAL_SIGNAL);
  digitalWrite(PIN_PORTAL_TRIGGER, HIGH);
}

/* Parsing data data dari database */
void parsingData(String arr_base_data[]) {
  int i = 0;
  while (http.available() || http.connected())
  {
    char c = http.read();
    if (c != '#') {
      arr_base_data[i] += c;
    } else if (c == '#') {
      i++;
    }
  }
}


/* Print Struk */
// ambil data id_barcode yang masih desimal lalu konversi ke ASCII dan cetak sesuai desain struk

void printerEpson(unsigned long id_barcode, String* jenis_layanan, String* harga)
{
  // debug
  printBarcode(converted_to_ascii, BARCODE_LENGTH); // Barcode : UPCA
  // Make sure USB printer found and ready
  if (uprinter.isReady()) {
    printer.begin();
    uprinter.write(27);
    uprinter.write(64); // init

    DPRINTLN(F("Init ESC POS printer"));

    printer.justify('c');
    printer.boldOn();
    printer.doubleHeightOn();
    printer.doubleWidthOn();
    printer.print("WELCOME \n");
    printer.print("BALI DRIVE TRHU CARWASH\n");
    printer.normal();
    printer.print("Jl. Mahendradata Selatan No.19 Denpasar, Bali\n\n");
    printBarcode(converted_to_ascii, BARCODE_LENGTH); // Barcode : UPCA
    printer.print("\n\n");

    printer.normal();
    printer.boldOff();
    printer.doubleHeightOn();
    printer.doubleWidthOn();
    printer.print(*jenis_layanan + "\n\n");
    printer.print("Rp.");
    printer.print(*harga);
    printer.feed(5);

    autoCutter();

  }
}

/* memotong kertas pada printer */
void autoCutter()
{
  uprinter.write(29);
  uprinter.write(86);
  uprinter.write(48);

}

/* Print Barcode */
void printBarcode(char data_id[], int len_id)
{
 
  printer.reset();

  // align: center
  uprinter.write(27);
  uprinter.write(97);
  uprinter.write(1); // rata tengah


  //Tinggi barcode
  uprinter.write(29);
  uprinter.write(104);
  uprinter.write(64); // tinggi barcode


  // posisi nomor barcode ( HRI)
  uprinter.write(29);
  uprinter.write(72);
  uprinter.write(2); // print nomor barcode di bawah barcode

  // pilih jenis tulisan HRI
  uprinter.write(29);
  uprinter.write(102);
  uprinter.write(1); //  tipe tulisan normal

  // lebar dari barcode
  uprinter.write(29);
  uprinter.write(119);
  uprinter.write(2); //  lebar

  //pre-cetak barcode
  uprinter.write(29);
  uprinter.write(107);
  uprinter.write(67); // tipe barcode: UPCA / EAN
  uprinter.write(BARCODE_LENGTH); // panjang angka barcode

  // cetak barcode
  for (int i = 0 ; i < len_id ; i++) {
    uprinter.write(converted_to_ascii[i]);
  }
  delay(100);

}

/* Convert decimal to ASCII */
// data konversi dari desimal ke ASCII
void convertToAscii(unsigned long number, int len_number)
{
  // convert to ASCII for Barcode
  int index = 0;
  while (number > 0) {
    raw_data[index++] = char((number % 10) + 48);
    number = number / 10;

  }
  DPRINT("Barcode sebelum di-reverse: ");
  DPRINTLN(raw_data);
  // reverse array from end to front
  int y = 0;
  for (int i = BARCODE_LENGTH - 1 ; i >= 0; i--)
  {
    if (raw_data[i] == NULL)
      converted_to_ascii[y] = '0';
    else
      converted_to_ascii[y] = raw_data[i];
    y++;
  }


}

/* LED Status properties */
void statusLedOn()
{
  digitalWrite(PIN_LED, HIGH);
}

void statusLedOff()
{
  digitalWrite(PIN_LED, LOW);
}
