#include <stdio.h>
#include <string.h>
#include "xtimer.h"
#include "ccs811.h"
#include "Include/ccs811_params.h"
#include "thread.h"
#include "fmt.h"
#include "periph/rtc.h"
#include "net/loramac.h"
#include "semtech_loramac.h"
#include "cayenne_lpp.h"
#include "config.h"


ccs811_t sensor;

kernel_pid_t sender_pid;
char sender_stack[LORAMAC_SEND_MSG_QUEUE];

extern semtech_loramac_t loramac;
static cayenne_lpp_t lpp; // data cointainer

//----------------------------------Sending thread function definition-----------------------
static void *sender(void * data)
{

	/* Thread gérant l'envoi via le protocole LoRaWAN
	* Stock dans l'objet cayenne la moyenne de toutes les valeurs mesurés par le capteur.
	* Reset tous les variables compteur(cnt) et les variables stockant les valeurs mesurés des
	* 2 capteurs et envoi la trame via LoRaWAN.
	*/

    (void)data;

    //LED1_ON; //LED Verte

    //Reset le lpp.buffer
    while(1){
    	cayenne_lpp_reset(&lpp);
    	data_sensor_reception(*data);
    	_send_message();
    {
    return NULL;
}

void sensor_data_init(sensor_data data){ // à mettre dans un .c de fonctions
  data.co2=0;
  data.tvoc=0;
}



static const char *message = "This is RIOT!";

//--------------------------------------Useful functions-------------------------------------------
//static void _prepare_next_alarm(void)
//{
  //  struct tm time;
   // rtc_get_time(&time);
    /* set initial alarm */
   // time.tm_sec += PERIOD;
  //  mktime(&time);
   // rtc_set_alarm(&time, rtc_cb, NULL);
//}

void _send_message(void)
{
    /* Envoi un message via le protocole LoRaWAN
    */

    /* Try to send the message */
    uint8_t ret = semtech_loramac_send(&loramac,lpp.buffer,lpp.cursor);

    if (ret != SEMTECH_LORAMAC_TX_DONE)
    {
        printf("Cannot send message, ret code: %d\n", ret);
        //LED1_OFF; //LED Verte
        //LED3_ON; //LED Rouge
    }
}

int connection_setup(void) // process to be executed once when joining application over OTAA
{
    puts("connection setup process ongoing");
    puts("=====================================");

    /* Convert identifiers and application key */
    fmt_hex_bytes(deveui, DEVEUI);
    fmt_hex_bytes(appeui, APPEUI);
    fmt_hex_bytes(appkey, APPKEY);

    /* Initialize the loramac stack */
    semtech_loramac_init(&loramac);
    semtech_loramac_set_deveui(&loramac, deveui);
    semtech_loramac_set_appeui(&loramac, appeui);
    semtech_loramac_set_appkey(&loramac, appkey);

    /* Use a fast datarate, e.g. BW125/SF7 in EU868 */
    semtech_loramac_set_dr(&loramac, LORAMAC_DR_5);

    /* Start the Over-The-Air Activation (OTAA) procedure to retrieve the
     * generated device address and to get the network and application session
     * keys.
     */
    puts("Starting join procedure");
    if (semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA) != SEMTECH_LORAMAC_JOIN_SUCCEEDED) {
        puts("Join procedure failed");
        return 1;
    }
    puts("Join procedure succeeded");

}

int data_sensor_initialization(ccs811_t sensor){

  puts("+------------Sensor Initialization------------+\n");
  /* initialize the sensor with default configuration parameters */

  if (ccs811_init (&sensor, &ccs811_params[0]) != CCS811_OK) {
      puts("Initialization failed\n");
      return 0;
  }

  puts("------------Setting up sensors parameters-----------\n");

  ccs811_set_environmental_data (&sensor, TEMPERATURE, HUMIDITY);
  return 1;
}

void data_sensor_reception(sensor_data data){

  switch(ccs811_power_up(&sensor)) {
    case CCS811_OK: // ou diff ? not ça
      if (ccs811_data_ready(&sensor) != CCS811_OK && ccs811_read_iaq(&sensor,&(data.tvoc), &(data.co2), 0, 0) != CCS811_OK) { // diff ou égal ?
          printf("TVOC [ppb]: %d\neCO2 [ppm]: %d\n", data.tvoc, data.co2);
          puts("+-------------------------------------+");
          cayenne_lpp_add_digital_input(&lpp,1,(uint8_t)(data.co2));
          cayenne_lpp_add_digital_input(&lpp,2,(uint8_t)(data.tvoc));
      }
    break;

    default:
        printf("Could not read data from sensor\n");
    break;
  }
  if (ccs811_power_down(&sensor) != CCS811_OK)
  {
    puts("Could not power down the device");
  }
  return;
}

int main(void)
{
    int ok=0;
    ccs811_t sensor;
    sensor_data data;

    sensor_data_init(data);

    xtimer_sleep(10); // short time to wait for the program to start
    // avoid error when lauching pyterm
    connection_setup();
    puts("CCS811 test application\n");

    ok=data_sensor_initialization(sensor);
    switch(ok){
      case 1:
      	printf("\n+--------Starting Measurements--------+\n");

      	sender_pid = thread_create(sender_stack, sizeof(sender_stack),THREAD_PRIORITY_MAIN, 0, sender, NULL, "sender"); // définition du processus
      break;
    default:
      printf("couldn't read data");
      // à faire

    }
    return 1;
  }





        /* wait and check for for new data every 10 ms */
        //while (ccs811_data_ready (&sensor) != CCS811_OK) {
        	//puts("ready failure\n");
         //   xtimer_sleep(2);
        //}

        /* read the data and print them on success */





  //  return 0;
//}
