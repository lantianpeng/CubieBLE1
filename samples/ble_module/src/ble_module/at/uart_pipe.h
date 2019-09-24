/** @file
 *  @brief Pipe UART driver header file.
 *
 *  A pipe UART driver that allows applications to handle all aspects of
 *  received protocol data.
 */

#include <uart.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Received data callback.
 *
 *  This function is called when new data is received on UART. The off parameter
 *  can be used to alter offset at which received data is stored. Typically,
 *  when the complete data is received and a new buffer is provided off should
 *  be set to 0.
 *
 *  @param buf Buffer with received data.
 *  @param off Data offset on next received and accumulated data length.
 *
 *  @return Buffer to be used on next receive.
 */
typedef u8_t *(*uart_pipe_recv_cb)(u8_t *buf, size_t *off);

/** @brief Send data over UART.
 *
 *  This function is used to send data over UART.
 *
 *  @param data Buffer with data to be send.
 *  @param len Size of data.
 *
 *  @return 0 on success or negative error
 */
int uart_pipe_send(const u8_t *data, int len);

/**
 * @brief Set the IRQ callback function pointer.
 *
 * This sets up the callback for IRQ. When an IRQ is triggered,
 * the specified function will be called.
 *
 * @param dev UART device structure.
 * @param cb Pointer to the callback function.
 *
 * @return N/A
 */
void uart_pipe_irq_callback_set(uart_irq_callback_t cb);

/** @brief UART Pipe init.
 *
 *  This function is used to init UART Pipe.
 *
 */
void uart_pipe_init(void);

#ifdef __cplusplus
}
#endif
