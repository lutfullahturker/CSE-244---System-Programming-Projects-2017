SearchWordAndWriteFile  1. ödevdeki tek dosyadan arama işlemini yapan programım.Tabi 1. ödevdeki halini değiştirdim program içinde log dosyası oluşturup yazıyorum ve toplam countu tutmak için her seferinde count.log isimli bir dosyaya o dosyadaki sayıyı yazıyorum.

Main programımda dosya bulunursa execl fonksiyonu ile SearchWordAndWriteFile programını child process te çağırıyorum.Klasör içinde klasör bulunursa recursive ile kendini bir daha çağırıyorum bu işlemi de child processe yaptırıyorum.

Main programının sonunda, oluşturduğum count.log dosyasını okuyup her integeri alıp toplayıp toplam halini log dosyasının sonuna yazıyorum.Ardından count.log dosyası ile işimiz bittiği için dosyayı siliyorum ve kullanıcı bu dosyanın varlığından bile haberdar olmuyor.
