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
