/* Header file */
// Programmer: I Putu Pawesi Siantika, S.T.
// Created   : Agustus 2022
// Tentang   : Program ini ditujukan untuk pengganti PC pada sistem potal pintu masuk
// Fitur     : Print struk (barcode), integrasi dengan database lokal, buka portal, terima masukkan dari user 
/* Konfigurasi:   
 *  1. Pastikan IP alat ini tidak konflik dengan IP perangkat lain yang ada di lokal server (baris 40, main code)
 *  2. Pastikan HOST server terisi dengan benar (baris 45, main code)
 *  3. Pastikan path_base_data dan path_last_id sesuai dengan konfigurasi:
 *    # path_base_data: alamat untuk mengambil jenis - jenis layanan  baris 46, main code
 *    # path_last_id  : alamat untuk melihat id terakhir (ini juga untuk post data dari alat ke server) baris 47, main code
    4. PENTING: protokol HTTP tolong disesuaikan di baris 234 - 245 , main code !
    5. Server lokal Harus dijalankan terlebih dahulu sebelum menjalankan alat ini !
*/

#define DEBOUNCE_TIME 100 // 50 ms
#define TOTAL_INDEX 8 // jumlah data " jenis_layanan " dari database
#define DELAY_TIME_FOR_PORTAL_SIGNAL 500 //ms
#define BARCODE_LENGTH 1 // EAN 12/13

// Debug console
#define DEBUG // uncomment untuk menampilakan serial debug.

#ifdef DEBUG
#define DPRINT(...) Serial.print(__VA_ARGS__)     
#define DPRINTLN(...) Serial.println(__VA_ARGS__) 
#else
#define DPRINT(...)   
#define DPRINTLN(...) 
#endif
