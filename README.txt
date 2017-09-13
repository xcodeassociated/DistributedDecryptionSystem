Pliki zawarte w tym archiwum są częścią składową projektu wykonanego na potrzeby pracy licencjackiej na wydziale Fizyki, Astronomii i Informatyki Stosowanej Uniwersytetu Jagiellońskiego.
Pracę wykonał Janusz Majchrzak, student 3 roku kierunku: Informatyka.

Projekt programistyczny składa się z dwóch aplikacji. Pierwsza w folderze “DDS” zajmuje się przeprowadzaniem procesu brute-force w systemie rozproszonym na zaszyfrowanym uprzednio pliku przez aplikację znajdującą się w folderze “SimpleCrypt”. 
Do działania aplikacji DDS potrzebny jest więc zaszyfrowany plik. W celu jego przygotowania najpierw należy skompilować aplikację SimpleCrypt oraz dokonać procesy enkcypcji - aplikacja posiada help, dzięki któremu poszczególnej jej opcje są opisane. 

Po uprzednim przygotowaniu pliku należy skompilować i uruchomić aplikację znajdującą się w folderze DDS. 

Proces kompilacji jest identyczny dla obu aplikacji:
1. Wchodzimy do katalogu odpowiedniej aplikacji
2. Tworzymy katalog (sugeruję nazwę “build”), który będzie przechowywać pliki potrzebne w procesie budowania aplikacji, jak również samą zbudowaną aplikację
3. Wchodzimy do stworzonego katalogu, a następnie wykonujemy: 
	cmake .. -DCMAKE_C_COMPILER=cc -DCMAKE_CXX_COMPILER=c++ -DO="3"4. Po wygenerowaniu niezbędnych plików należy wykonać polecenie: 
	make all
5. Tak przygotowaną aplikację możemy uruchomić. 

Szczegółowe instrukcje odnośnie przygotowania środowiska na jakim będzie działać aplikacja oraz informacje jak zainstalować potrzebne biblioteki oraz aplikacje dla systemu Linux zawarłem w treści pracy dyplomowej. 
Znajdują się tam również informacje o tym jak uruchomić poszczególną aplikację oraz opisy działania każdej z nich.
