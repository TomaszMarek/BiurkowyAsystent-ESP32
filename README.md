# Biurkowy Asystent Klimatyczny v2.0 (ESP32)

Projekt inteligentnego asystenta biurkowego opartego na platformie ESP32 (TTGO). Urządzenie realizuje ciągły monitor warunków klimatycznych w pomieszczeniu, wykrywa obecność użytkownika w przestrzeni roboczej za pomocą czujnika ultradźwiękowego oraz raportuje stan systemu na zewnętrzny serwer Discord za pośrednictwem protokołu HTTP.

## Funkcje systemu
* **Zarządzanie energią i wyświetlaczem (Maszyna stanów):** Automatyczne wygaszanie ekranu i przejście w tryb uśpienia w przypadku braku detekcji ruchu przez okres powyżej 15 sekund.
* **Detekcja obecności:** Automatyczne wybudzenie układu i inicjalizacja ekranu powitalnego po wykryciu obiektu w zdefiniowanej strefie.
* **Telemetria środowiskowa:** Pomiar temperatury oraz wilgotności względnej w czasie rzeczywistym przy użyciu czujnika BME280.
* **Integracja sieciowa (Webhook):** Wysyłanie pakietów danych ze statystykami uruchomieniowymi oraz aktualnymi odczytami na interfejs API Discorda po każdym wybudzeniu urządzenia.

## Wymagania sprzętowe
* Mikrokontroler ESP32 (rekomendowany moduł TTGO T-Display ze zintegrowanym ekranem IPS TFT)
* Czujnik środowiskowy BME280 (magistrala I2C)
* Ultradźwiękowy czujnik odległości HC-SR04 lub kompatybilny (np. JSN-SR04T)

## Specyfikacja połączeń (Pinout)

### Czujnik odległości HC-SR04
| Pin HC-SR04 | Pin ESP32 TTGO | Funkcja |
| :--- | :--- | :--- |
| VCC | 5V | Zasilanie |
| Trig | GPIO 25 | Sygnał wyzwalający (Output) |
| Echo | GPIO 26 | Sygnał zwrotny (Input) |
| GND | GND | Masa układu |

### Czujnik środowiskowy BME280
| Pin BME280 | Pin ESP32 TTGO | Funkcja |
| :--- | :--- | :--- |
| VCC | 3V / 3.3V | Zasilanie |
| GND | GND | Masa układu |
| SCL | GPIO 22 | Linia zegarowa I2C |
| SDA | GPIO 21 | Linia danych I2C |

## Instrukcja wdrożenia i uruchomienia
1. Sklonuj lub pobierz zawartość tego repozytorium.
2. W katalogu głównym projektu utwórz plik konfiguracji lokalnej o nazwie `secrets.h` na podstawie udostępnionego szablonu `secrets.h.example`.
3. Wprowadź parametry sieciowe (SSID, hasło Wi-Fi) oraz pełny adres URL webhooka Discord w odpowiednie dyrektywy `#define` w pliku `secrets.h`.
4. Skonfiguruj plik nagłówkowy `User_Setup.h` w strukturze biblioteki `TFT_eSPI` zgodnie ze specyfikacją posiadanego kontrolera wyświetlacza.
5. Skompiluj i wgraj oprogramowanie na płytkę deweloperską za pomocą środowiska Arduino IDE lub PlatformIO.