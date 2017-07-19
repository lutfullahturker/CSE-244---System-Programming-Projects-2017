Main programımda dosya bulunursa execl fonksiyonu ile SearchWordAndWriteFile programını child process te çağırıyorum.Klasör içinde klasör bulunursa recursive ile kendini bir daha çağırıyorum bu işlemi de child processe yaptırıyorum.

SearchWordAndWriteFile  1. ödevdeki tek dosyadan arama işlemini yapan programım.Tabi 1. ödevdeki halini değiştirdim programı main.c de execl ile çalıştırmadan önce
pipe oluşturduktan sonra dup2 ile execl nin stdout unu pipe ın yazma ucuna ve programımın stdin ini de pipe ın okuma ucuna yönlendirince SearchWordAndWriteFile  ın çıktısını
pipe a yazmış oluyorum.main.c de de pipe dan alıp log dosyasına yazıyorum.

Fifo için de her klasör bulunduğunda o alt klasör için fifo oluşturuyorum.Her klasör kendi fifosuna aranan kelimenin toplam bulunma sayısını(count) yazıyor.
recursive çağrıyı çocuk yaparken de ana proses de alt klasörünün fifoya yazdığını üst klasörün fifosuna aktarıyor.recursive sonlandığında tüm alt klasörlerden gelen countlar ana klasörün
fifosunda bulunuyor.Ana klasörün fifosunu okuması için ayrı bir fonksiyon yazdım.Bu fonksiyon da fifo içindeki tüm integer sayıları okuyup += le toplayıp total e atıyor.
döngü sonunda fifoda okuyacak sayı kalmadığında da totali log dosyasının sonuna yazıyorum.Bu readFifo fonksiyonu programın başından sonuna kadar bir proses tarafından yapılıyor.
ana Fifonun write ucu kapatılana kadar program boyunca birşeyler yazılmasını bekleyerek okuyor.
