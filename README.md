# dmp - Device Mapper Proxy
Прокси драйвер блочных устройств, который собирает статистику.
Статистика собирается в `/sys/module/dmp/stat/volumes` и содержит информацию о количестве и чтений, записей. 
Также доступны средняя длина прочитанного и записанного блоков.


### Установка 
Установка проводилась для Linux Ubuntu-24.04.02 6.8.0-59-generic x86_64.

```bash
make CC=x86_64-linux-gnu-gcc-13  -C /usr/src/linux-headers-6.8.0-58-generic M=`pwd` modules
```
```bash
insmod dmp
```

### Тестирование
Тестирование проводилось на аналогичной машине, что и установка.

Создание тестового блочного устройства:
```bash
dd if=/dev/zero of=/tmp/disk1 bs=512 count=20000
```
```bash
losetup /dev/loop6 /tmp/disk1
```
Создание прокси:
```bash
dmsetup create dmp1 --table "0 20000 dmp /dev/loop6"
```
Нагрузка драйвера:
```bash
dd if=/dev/zero of=/dev/mapper/dmp1 count=1 bs=4K
```
Далее смотрим содержимое файла `/sys/module/dmp/stat/volumes` любой из доступных утилит.
