/* Copyright 2020, Franco Bucafusco
 * All rights reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*==================[inclusiones]============================================*/

// Includes de FreeRTOS
#include <stdio.h>
#include <stdarg.h>
#include "sapi.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"

#include "task.h"
#include "semphr.h"

/*==================[definiciones y macros]==================================*/

#define CUENTAS_1MS             (9251*2)
#define PERIOD_TIME             1000

/* en 1 usa semaforos en 0 usa mutex*/
#define EVIDENCIAR_PROBLEMA     1

#if EVIDENCIAR_PROBLEMA==1
#define CRITICAL_CONFIG     mutex = xSemaphoreCreateBinary()
#else
#define CRITICAL_CONFIG     mutex = xSemaphoreCreateMutex()
#endif

#define CRITICAL_DECLARE    SemaphoreHandle_t mutex
#define CRITICAL_START      xSemaphoreTake( mutex , portMAX_DELAY )
#define CRITICAL_END        xSemaphoreGive( mutex )

/*==================[definiciones de datos internos]=========================*/
typedef struct
{
    char* text;
    uint32_t led;
    uint32_t periods;
} t_params;

t_params params[] = { { "tarea a", LEDR, 1 }, {"tarea b", LED1, 3 }, { "tarea c", LED2, 5 }, { "tarea d", LED3, 1 } };

CRITICAL_DECLARE;

/*==================[definiciones de datos externos]=========================*/

print_t debugPrint;

/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/
TaskHandle_t task_handles_a;
TaskHandle_t task_handles_b;
TaskHandle_t task_handles_c;
TaskHandle_t task_handles_d;

// Prototipo de funcion de la tarea

TickType_t tini_system;

void tarea_iniciadora( void*  );
void tarea_A_code( void*  );
void tarea_D_code( void*  );
void tarea_BC_code( void*  );
void tarea_B_code( void*  );
void tarea_C_code( void*  );


/**
   @brief dimula un proceso de cierta duracion en donde se gasta CPU sin entrar en estado Blocked

   @param ms
 */
void proceso_ms( uint32_t ms )
{
    volatile uint32_t i;
    volatile uint32_t j;

    for( i=0 ; i<ms ; i++ )
    {
        /* delay de aprox 1 ms */
        for( j=0 ; j<CUENTAS_1MS ; j++ )
        {
        }
    }
}

/*==================[funcion principal]======================================*/

// FUNCION PRINCIPAL, PUNTO DE ENTRADA AL PROGRAMA LUEGO DE ENCENDIDO O RESET.
int main( void )
{
    BaseType_t res;

    // ---------- CONFIGURACIONES ------------------------------
    printf( "\nEjercicio D5\n" );

    if( EVIDENCIAR_PROBLEMA )
    {
        printf( "Ejecucion usando semaforos\n" );
    }
    else
    {
        printf( "Ejecucion usando mutex\n" );
    }

    // Crear tarea en freeRTOS
    res = xTaskCreate(
              tarea_iniciadora,
              ( const char * )"tarea_init",
              configMINIMAL_STACK_SIZE*2,
              NULL,
              tskIDLE_PRIORITY+5,
              NULL
          );

    configASSERT( res == pdPASS );

    // Iniciar scheduler
    vTaskStartScheduler();

    // ---------- REPETIR POR SIEMPRE --------------------------
    while( 1 )
    {
        // Si cae en este while 1 significa que no pudo iniciar el scheduler
    }

    // NO DEBE LLEGAR NUNCA AQUI, debido a que a este programa se ejecuta
    // directamenteno sobre un microcontroladore y no es llamado por ningun
    // Sistema Operativo, como en el caso de un programa para PC.
    return 0;
}

/*==================[definiciones de funciones internas]=====================*/


/**
   @brief blink bloqueante.

   @param n
   @param led
 */
void blink_n_500( uint32_t n, uint32_t led )
{
    /* genero 2 blinks*/
    int blink_count  = n;
    int cycles       = blink_count*2;

    for( ; cycles>0 ; cycles-- )
    {
        gpioToggle( led );
        proceso_ms( PERIOD_TIME / 2 );
    }
}

/*==================[definiciones de funciones externas]=====================*/
void tarea_iniciadora( void* taskParmPtr )
{
    BaseType_t res;

    UBaseType_t my_prio = uxTaskPriorityGet( 0 );   /* se podria haber usado uxTaskPriorityGet( task_handles[0] ) */

    /* Creo las tareas A B C y D.
       No hay cambios de contexto porque la esta tarea tiene mayor prioridad de las creadas
    */
    res = xTaskCreate(
              tarea_A_code,
              ( const char * ) "tarea_a",
              configMINIMAL_STACK_SIZE*3,
              &params[0],                               /* Parametros de tarea */
              my_prio - 3,                              /* minima prioridad del sistema */
              &task_handles_a                           /* handle de la tarea */
          );

    res = xTaskCreate(
              tarea_C_code,
              ( const char * ) "tarea_c",
              configMINIMAL_STACK_SIZE*3,
              &params[2],                               /* Parametros de tarea */
              my_prio - 2,                              /* prioridad media del sistema */
              &task_handles_c                           /* handle de la tarea */
          );

    configASSERT( res == pdPASS );

    res = xTaskCreate(
              tarea_B_code,
              ( const char * ) "tarea_b",
              configMINIMAL_STACK_SIZE*3,
              &params[1],                               /* Parametros de tarea */
              my_prio - 2,                              /* prioridad media del sistema */
              &task_handles_b                           /* handle de la tarea */
          );

    configASSERT( res == pdPASS );

    res = xTaskCreate(
              tarea_D_code,
              ( const char * )"tarea_d",
              configMINIMAL_STACK_SIZE*3,
              &params[3],                               /* Parametros de tarea */
              my_prio - 1,                              /* prioridad alta del sistema */
              &task_handles_d                           /* handle de la tarea */
          );

    configASSERT( res == pdPASS );


    CRITICAL_CONFIG;

#if EVIDENCIAR_PROBLEMA==1
    /* en este caso, lo libero para poder user al semaforo binario como control de seccion critica.  */
    xSemaphoreGive( mutex );
#endif

    /* ya cumpli mi mision */
    vTaskDelete( 0 );
}

void tarea_B_code( void* taskParmPtr )
{
    tarea_BC_code( taskParmPtr );

    /* me auto destruyo */
    vTaskDelete( 0 );
}


void tarea_C_code( void* taskParmPtr )
{
    tarea_BC_code( taskParmPtr );

#if EVIDENCIAR_PROBLEMA==0
    uint32_t dif = xTaskGetTickCount() - tini_system;
    printf( "tiempo total %u\n", dif );
    fflush(stdout); //se flushea el stdout antes de matar a la tarea.
#endif

    /* me auto destruyo */
    vTaskDelete( 0 );
}

void tarea_BC_code( void* taskParmPtr )
{
    t_params * param = ( t_params* ) taskParmPtr;

    /* bloqueo la tarea, para que la tarea A tome el CPU */
    vTaskDelay( 101 / portTICK_RATE_MS );

    TickType_t tini = xTaskGetTickCount();

    blink_n_500( param->periods , param->led );
    //proceso_ms( param->periods * PERIOD_TIME );

    printf( "Soy %s y tarde en ejecutarme %u ms \n", param->text, xTaskGetTickCount()-tini );
}

void tarea_AD_common( void* taskParmPtr )
{
    t_params * param = ( t_params* ) taskParmPtr;

    TickType_t tini = xTaskGetTickCount();

    printf( "Hola ! Soy %s y quiero usar un recurso\n", param->text );

    CRITICAL_START;

    printf( "Soy %s y tarde %u ms en acceder al recurso\n", param->text, xTaskGetTickCount()-tini );

    /* ciclos de CPU simulando un procesamiento de un cierto tiempo para la tarea */
     blink_n_500( param->periods , param->led );
    //proceso_ms( param->periods * PERIOD_TIME );

    printf( "Chau ! Soy %s y tarde en ejecutarme %u ms \n", param->text, xTaskGetTickCount()-tini );

    CRITICAL_END;
}

void tarea_A_code( void* taskParmPtr )
{
    tini_system =  xTaskGetTickCount();
    tarea_AD_common( taskParmPtr );

    /* me auto destruyo */
    vTaskDelete( 0 );
}

void tarea_D_code( void* taskParmPtr )
{
    /* bloqueo la tarea, para que la tarea A tome el CPU */
    vTaskDelay( 100 / portTICK_RATE_MS );

    tarea_AD_common( taskParmPtr );

#if EVIDENCIAR_PROBLEMA==1
    printf( "tiempo total %u ms\n", xTaskGetTickCount() - tini_system );
    fflush(stdout); //se flushea el stdout antes de matar a la tarea.
#endif

    /* me auto destruyo */
    vTaskDelete( 0 );
}

/*==================[fin del archivo]========================================*/
