Number of cascade threads created :  bölümünde her klasör içinde oluşturulan toplam thread sayılarını yazmamız
isteniyor şeklinde anladım.Ve print yaparken her klasörde kaç thread oluştuğunu yazdım.Hangi klasör olduğu anlaşılsın diye her klasörün path i ve içinde oluşan thread sayısını yazıyorum.Bu yüzden çıktıda klasör sayısı kadar satır print yapılıyor.

Oluşan toplam thread sayısı her dosya için thread oluşturulacağı için toplam dosya sayısına eşit oluyor.

Max # of threads running concurrently. Bölümünde aynı anda oluşabilecek toplam thread sayısını toplam klasör sayısı olarak yazdım.Çünkü her klasörde dosya sayısı kadar thread  oluşuyor fakat aynı klasördeki her yeni thread semaforun wait post kısmı ile ayrı zamanlarda çalışıyor.Aynı anda çalışabilen threadler sadece farklı klasörlerdeki threadler olmuş oluyor.Bu yüzden de maximum aynı anda çalışabilen thread sayısı toplam klasör sayısı kadar olabiliyor.

Toplam oluşan shared memory sayısı her klasör için ayrı shared memory oluşturulacağı için toplam klasör sayısına eşittir.

Her shared memorynin size'ı sizeof(int) kadar olur.Çünkü her klasörde bir shared memory açıyorum(sizeof(int) boyutunda) ve paylaştığım integer ı  += kullanarak her dosyada toplam bulunan kelime sayısını üstüne ekleyerek topluyorum.

Message Queue için Hoca derste ödevi anlatırken her klasör root ile iletişim kuracak şeklinde bir cümle de kurdu.Her klasör bir üst klasöre aktara aktara en üst klasöre kadar mesajı ulaştıracak şeklinde bir cümle de kurdu.Ben tek message queue oluşturup tüm klasörlerin o msgqueue ye yazmasını daha kolay olduğunu düşündüğüm için 3. ödevdeki gibi her klasördeki bilgiyi bir üst klasöre message queue ile aktararak yaptım.Tüm alt klasörler bir üstüne aktara aktara en üst klasörde hepsi toplanmış oluyor.

Programı make yapıp  ./grepSh string testFolder şeklinde çalıştırınız.
