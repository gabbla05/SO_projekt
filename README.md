# SO_projekt

# Salon Fryzjerski

## Autor: BŁAUT GABRIELA
### Nr albumu: 151405

## Opis projektu
Projekt stanowi rozszerzenie problemu śpiącego fryzjera/golibrody na wielu fryzjerów. 

W salonie pracuje **F fryzjerów** (F > 1) i znajduje się w nim **N foteli** (N < F). 
Salon jest czynny w godzinach od **Tp** do **Tk**. 
Klienci przychodzą do salonu w losowych momentach czasu. 
W salonie znajduje się poczekalnia, która może pomieścić **K** klientów jednocześnie. 
Każdy klient rozlicza usługę z fryzjerem przekazując mu kwotę za usługę przed rozpoczęciem strzyżenia. 
Fryzjer wydaje resztę po zakończeniu obsługi klienta a w przypadku braku możliwości wydania reszty klient musi zaczekać, aż fryzjer znajdzie resztę w kasie. 
Kasa jest wspólna dla wszystkich fryzjerów. 
Płatność może być dokonywana banknotami o nominałach 10zł, 20zł i 50zł.

### Zasady działania
#### Fryzjer (cyklicznie):
1. wybiera klienta z poczekalni lub czeka jeśli go jeszcze nie ma;
2. znajduje wolny fotel;
3. pobiera opłatę za usługę i umieszcza we wspólnej kasie (opłata może być również przekazana do wspólnej kasy bezpośrednio przez klienta, ale fryzjer musi znać kwotę, żeby wyliczyć resztę do wydania);
4. realizuje usługę;
5. zwalnia fotel;
6. wylicza resztę i pobiera ze wspólnej kasy, a jeśli nie jest to możliwe, czeka, aż pojawią się odpowiednie nominały w wyniku pracy innego fryzjera;
7. przekazuje resztę klientowi.

#### Klient (cyklicznie):
1. zarabia pieniądze;
2. przychodzi do salonu fryzjerskiego;
3. jeśli jest wolne  miejsce w poczekalni, siada i czeka na obsługę (ewentualnie budzi fryzjera), a w przypadku braku miejsc opuszcza salon i wraca do zarabiania pieniędzy;
4. po znalezieniu fryzjera płaci za usługę;
5. czeka na zakończenie usługi;
6. czeka na resztę;
7. opuszcza salon i wraca do zarabiania pieniędzy.

#### Kasjer (kierownik):
**sygnał 1** dany fryzjer kończy pracę przed zamknięciem salonu.
**sygnał 2** wszyscy klienci (ci, którzy siedzą na fotelach i w poczekalni) natychmiast opuszczają salon.

## Wymagania
1. **Implementacja wielowątkowości**:
   - Wątki dla fryzjerów i klientów.
   - Synchronizacja przy użyciu mechanizmów, takich jak semafory, mutexy.
2. **Obsługa wspólnej kasy**
   - Jednoczesny dostęp wielu fryzjerów.
   - Gwarancja poprawności wyliczeń reszty.
3. **Walidacja wejścia**
   - Losowy przydział nominałów (10zł, 20zł, 50zł) dla klientów.
   - Weryfikacja dostępności reszty
  
## Testy
1. **Test wydajności**
   - Liczba klientów większa niż pojemność poczekalni.
   - Sprawdzaenie, czy nadmiarowi klienci opuszczają salon.
2. **Test synchronizacji**
   - Fryzjerzy jednocześnie korzystają z kasy.
   - Weryfikacja poprawności obsługi transakcji.
3. **Test braku reszty**
   - Brak odpowiednich nominałów w kasie.
   - Fryzjerzy czekają na uzupełnienie kasy.
4. **Test skrajnych przypadków**
   - Brak klientów przez dłuższy czas.
   - Klienci przychodzą w krótkich odstępach czasu.
5. **Test sygnałów kierownika**
   - Sygnał 1: Fryzjer kończy pracę, pozostali kontynuują.
   - Sygnał 2: Wszyscy klienci opuszczają salon.
6. **Test masowy**
   - Duża liczba klientów i fryzjerów.
   - Weryfikacja poprawności obsługi poczekalni i kasy.
7. **Test minimalnej konfiguracji**
   - Jeden fryzjer, jeden fotel, brak poczekalni.
   - Sprawdzenie poprawności obsługi klientów.
8. **Test opóźnień w pracy fryzjera**
   - Dłuższy czas strzyżenia dla jednego fryzjera.
   - KLienci kierowani do innych dostępnych fryzjerów.
9. **Test wielokrotnych wizyt klientów**
   - Klient opuszcza salon i wraca po losowym czasie.
   - Weryfikacja obsługi powtarzających się wizyt.
10. **Test poprawności płatności**
   - KLienci płacą różnymi nominałami.
   - Sprawdzenie poprawności wydawania reszty.
11. **Test równowagi pracy frzyjerów**
   - Równomierne rozdzielanie klientów między fryzjerów.
   - Weryfikacja, czy żaden fryzjer nie jest przeciążony.
12. **Test awaryjnego zamknięcia salonu**
   - Wysłanie sygnału zamknięcia w trakcie obsługi klientów.
   - Fryzjerzy kończą pracę, a klienci opuszczają salon

## Repozytorium

https://github.com/gabbla05/SO_projekt
