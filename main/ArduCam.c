/*
 *		MIT License
 *
 *	Copyright (c) 2022 SelfTide
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a copy of this
 *	software and associated documentation files (the "Software"), to deal in the Software
 *	without restriction, including without limitation the rights to use, copy, modify, merge,
 *	publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
 *	to whom the Software is furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be included in all copies or
 *	substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 *	BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 *	DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "ArduCam.h"
#include "ov2640_regs.h"

static const char *TAG = "ArduCAM";
extern ArduCAM_OV2640_C *myCAM;

// Función para medir el tiempo transcurrido en milisegundos
// entre dos puntos en el código.
static int64_t eclipse_time_ms(bool startend)
{
    // Enumeración para identificar el inicio y fin del temporizador.
    enum
    {
        START = 0,
        END,
    };

    // Variables estáticas para retener el tiempo de inicio y fin.
    static struct timeval tik, tok;

    // Iniciar el temporizador.
    if (startend == START)
    {
        gettimeofday(&tik, NULL);
        return 0; // Indicar que el temporizador ha comenzado.
    }
        // Detener el temporizador y calcular la diferencia de tiempo.
    else
    {
        gettimeofday(&tok, NULL);
        int64_t time_ms = (int64_t)(tok.tv_sec - tik.tv_sec) * 1000 + (tok.tv_usec - tik.tv_usec) / 1000;
        return time_ms; // Devolver el tiempo transcurrido en milisegundos.
    }
}


void OV2640_valid_check()
{
    // Verificar la integridad del bus SPI de ArduCAM
    ArduCAM_write_reg(ARDUCHIP_TEST1, 0x55);
    uint8_t temp = ArduCAM_read_reg(ARDUCHIP_TEST1);
    if (temp != 0x55)
        ESP_LOGE(TAG, "SPI: Error en la interfaz 0x%2x!\n", temp);
    else
        ESP_LOGI(TAG, "SPI: Interfaz OK: %x\n", temp);

    // Verificar el tipo de módulo de cámara OV2640
    uint8_t vid, pid;
    ArduCAM_wrSensorReg8_8(0xff, 0x01);
    ArduCAM_rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
    ArduCAM_rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
    if ((vid != 0x26) && ((pid != 0x41) || (pid != 0x42)))
        ESP_LOGE(TAG, "I2C: No se puede encontrar el módulo OV2640!\n");
    else
        ESP_LOGI(TAG, "I2C: OV2640 detectado.\n");
}


void capture_one_frame()
{
    // Vaciar el FIFO de la cámara
    ArduCAM_flush_fifo();
    // Limpiar la bandera de FIFO
    ArduCAM_clear_fifo_flag();
    // Iniciar la captura del fotograma
    ArduCAM_start_capture();

    // Iniciar el temporizador para medir el tiempo de captura
    eclipse_time_ms(false);
    // Esperar a que se complete la captura del fotograma
    while (!(ArduCAM_get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))) taskYIELD();
    // Detener el temporizador y mostrar el tiempo transcurrido
    ESP_LOGI(TAG, "Tiempo total de captura (en milisegundos): %lld\n", eclipse_time_ms(true));

}

void read_frame_buffer(void)
{
	video_stream_packet_state *vsps;
	data_pack *dpack;
	uint8_t *buffer;
	uint8_t temp_count = 0;
	size_t len = 0, batch_num = 0, tail_len = 0;

	// ESP_LOGI(TAG, "start_capture = %s\t", start_capture ? "true" : "false");
	// ESP_LOGI(TAG, "stack_count = %d\t", stack_count);

    // Verificar si se puede procesar más datos en la pila
    if(stack_count < 5)
	{
        // Obtener el tamaño del búfer de imagen
        len = ArduCAM_read_fifo_length(myCAM);
		batch_num = len / SPI_MAX_TRANS_SIZE;
		tail_len = len % SPI_MAX_TRANS_SIZE;

		// ESP_LOGI(TAG, "in read_frame_buffer\n");

        // Comprobar el tamaño del búfer
        if (len >= MAX_FIFO_SIZE)
		{
			ESP_LOGE(TAG, "Over size.\n");
			return;
		}else if (len == 0){
			ESP_LOGE(TAG, "Size is 0.\n");
			return;
		}

		// Comienzo de la información
		vsps = (video_stream_packet_state *)calloc(1, sizeof(video_stream_packet_state));
		vsps->start_image = true, vsps->data_image = false, vsps->end_image = false;
		vsps->image_size = len;
			
		dpack = (data_pack *)calloc(1, sizeof(data_pack));
		dpack->data = vsps;
		dpack->length = sizeof(video_stream_packet_state);
		
		push_stack(dpack);
		temp_count++;

		// Envio de la Informacion
		vsps = (video_stream_packet_state *)calloc(1, sizeof(video_stream_packet_state));
		vsps->start_image = false, vsps->data_image = true, vsps->end_image = false;
		vsps->image_size = len;
		
		dpack = (data_pack *)calloc(1, sizeof(data_pack));
		dpack->data = vsps;
		dpack->length = sizeof(video_stream_packet_state);
		
		push_stack(dpack);
		temp_count++;

        // leer y envia en bytes de la cámara hasta que termine, también asegúrese de que no tengamos una imagen pequeña
        for (int i = 0; i < batch_num && batch_num > 0; i++)
		{
			dpack = (data_pack *)calloc(1, sizeof(data_pack));
			buffer = (uint8_t *)calloc(SPI_MAX_TRANS_SIZE, sizeof(uint8_t));	// allocate buffer

            // Leer datos del búfer de la cámara
			ArduCAM_CS_LOW(myCAM);
			dpack->length = spi_transfer_bytes(BURST_FIFO_READ, buffer, buffer, SPI_MAX_TRANS_SIZE);	// read in bytes to buffer
			ArduCAM_CS_HIGH(myCAM);
			dpack->data = buffer;
			
			push_stack(dpack);
			temp_count++;
		}

        // Leer y enviar datos restantes
		if (tail_len != 0)	
		{
			dpack = (data_pack *)calloc(1, sizeof(data_pack));
			buffer = (uint8_t *)calloc(tail_len, sizeof(uint8_t));
			ArduCAM_CS_LOW(myCAM);
			dpack->length = spi_transfer_bytes(BURST_FIFO_READ, buffer, buffer, tail_len);	// read in bytes to buffer
			ArduCAM_CS_HIGH(myCAM);
			ESP_LOGI(TAG, "4 Byte header");
			ESP_LOGI(TAG, "Byte[0]: %x", buffer[0]);
			ESP_LOGI(TAG, "Byte[1]: %x", buffer[1]);
			ESP_LOGI(TAG, "Byte[2]: %x", buffer[2]);
			ESP_LOGI(TAG, "Byte[3]: %x", buffer[3]);
			ESP_LOGI(TAG, "Burst size: %d", dpack->length);
			dpack->data = buffer;
			
			push_stack(dpack);
			temp_count++;
		}
		 
		// Enviar paquete de fin
		vsps = (video_stream_packet_state *)calloc(1, sizeof(video_stream_packet_state));
		vsps->start_image = false, vsps->data_image = false, vsps->end_image = true;
		vsps->image_size = 0;
		
		dpack = (data_pack *)calloc(1, sizeof(data_pack));
		dpack->data = vsps;
		dpack->length = sizeof(video_stream_packet_state);
		
		push_stack(dpack);
		temp_count++;

        // Actualizar el contador de la pila
		while(pthread_mutex_lock(&lock) != 0) taskYIELD();
		stack_count += temp_count;
		pthread_mutex_unlock(&lock);
		
	}
}

void take_image(void *arg)
{
	ESP_LOGI(TAG, "In take_image()");

	while(1)
	{
		if(start_capture)
		{
			// ESP_LOGI(TAG, "getting image...\n");
			capture_one_frame();
			read_frame_buffer();
		}else{
			vTaskDelay(pdMS_TO_TICKS(10));
		}
	}
}
