
#ifndef MILLIS_H_
#define MILLIS_H_

/**
* Milliseconds data type \n
* Data type				- Max time span			- Memory used \n
* unsigned char			- 255 milliseconds		- 1 byte \n
* unsigned int			- 65.54 seconds			- 2 bytes \n
* unsigned long			- 49.71 days			- 4 bytes \n
* unsigned long long	- 584.9 million years	- 8 bytes
*/
typedef unsigned long millis_t;

#define millis() millis_get()
#define TIKS_1MS (F_CPU/1024/1000)		// Размер одного тика таймера, заменяющего 1 мс в delay_ms

#ifdef __cplusplus
extern "C" {
#endif

void millis_init(void);
millis_t millis_get(void);


#ifdef __cplusplus
}
#endif

#endif