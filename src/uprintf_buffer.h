#ifndef __UPRINTF_BUFFER_H__
#define __UPRINTF_BUFFER_H__


void uprintf_buffer_init(void);

void uprintf_buffer_putchar(unsigned int uiChar);
void uprintf_buffer_flush(void);

void uprintf_buffer_putrawchar(unsigned char ucChar);


#endif  /* __UPRINTF_BUFFER_H__ */
