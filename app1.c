#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

int main(int argc, char* argv[argc+1]){

    FILE *archivo;
    FILE *archivo1;
    uint8_t data[12];
    struct dim16b{
        int16_t x;
        int16_t y;
        int16_t z;
    };
    struct dim16b accel;
    struct dim16b gyro;
    struct dimd{
        float x;
        float y;
        float z;
    };
    struct dimd ang;

    double dt;
    time_t time1;
    time(&time1);
    time_t time2;

    float pitch = 0;
    float roll = 0;
    float atotal = 0;

    size_t pitch_0 = 0;
    size_t roll_0 = 0;

    size_t pitch_pos = 0;
    size_t roll_pos = 0;

    size_t pitch_neg = 0;
    size_t roll_neg = 0;

    int flag = 0;
    int impulse = 0;
    int wait = 0;
    int impulse_d = 0;

    char direccion;
    char impulso;
    char buff[2];

    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = 32L;

    system("sudo chmod 777 /dev/led_SIE");

    while(1){

        archivo = fopen("/dev/led_SIE", "r");
        if(archivo == NULL){
            printf("No existe el archivo");
            return EXIT_FAILURE;
        }

        char lee[13];

        fread(lee, sizeof(char), sizeof(lee), archivo);

        fclose(archivo);
        for(size_t i = 0; i < 12; ++i){
            data[i] = (unsigned char)lee[i];
            /*printf("%X\n", data[i]);*/
        }

        time(&time2);
        dt = difftime(time1,time2);
        time(&time1);
        if(dt < 0.0)
            dt = 0.0;

        accel.x = ((data[0] << 8) | data[1]);
        accel.y = (data[2] << 8) | data[3];
        accel.z = ((data[4] << 8) | data[5]);

        gyro.x = (data[6] << 8) | data[7];
        gyro.y = (data[8] << 8) | data[9];
        gyro.z = (data[10] << 8) | data[11];

        ang.x = (180/3.141592) * atan((float)accel.x / sqrt(pow((float)accel.y, 2) + pow((float)accel.z, 2)));
        /*ang.x = (180/3.141592) * atan2f((float)accel.y, (float)accel.z); //roll*/
        ang.y = (180/3.141592) * atan((float)accel.y / sqrt(pow((float)accel.x, 2) + pow((float)accel.z, 2)));
        /*ang.y = (180/3.141592) * atan2f(-(float)accel.x, (float)accel.z); //pitch*/
        ang.z = (180/3.141592) * atan(sqrt(pow(accel.y, 2) + pow(accel.x, 2)) / accel.z);

        pitch = 0.89 * (pitch + ((float)(gyro.x)/65.536) * (float)dt) + 0.11 * ang.x;
        roll = 0.89 * (roll + ((float)(gyro.y)/65.536) * (float)dt) + 0.11 * ang.y;

        atotal = sqrt(accel.x*accel.x+accel.y*accel.y+accel.z*accel.z);


        /*printf("%i\t%f\t%f\t%f\t%i\t%i\t%i\n",samples,pitch,roll,atotal,accel.x,accel.y,accel.z);*/

        pitch_0 = (pitch < 25) && (pitch > -25);
        roll_0 = (roll < 25) && (roll > -25);
        pitch_pos = (pitch > 45) && (pitch < 100);
        roll_pos = (roll > 45) && (roll < 100);
        pitch_neg = (pitch < -45) && (pitch > -100);
        roll_neg = (roll < -45) && (roll > -100);

        /*printf("%i\t%i\t%i\t%i\t%i\t%i\n", pitch_0, roll_0, pitch_pos, roll_pos, pitch_neg, roll_neg);*/

        if(pitch_0 && roll_0){
            buff[0] = '0';
        } else if(roll_0 && pitch_pos){
            /*printf("derecha\n");*/
            buff[0] = 'D';
        } else if(roll_0 && pitch_neg){
            /*printf("izquierda\n");*/
            buff[0] = 'A';
        } else if(pitch_0 && roll_pos){
            /*printf("adelante\n");*/
            buff[0] = 'W';
        } else if(pitch_0 && roll_neg){
            /*printf("atras\n");*/
            buff[0] = 'S';
        }

        if(atotal > 30000 && impulse == 0){
            impulse = 1;
        }

        if(impulse && (accel.x > 24000) && (wait == 0)){
            impulse_d = 1;
            impulse = 0;
            flag=1;
        }

        if(impulse && (accel.x < -24000) && (wait == 0)){
            impulse_d = 2;
            impulse = 0;
            flag=1;
        }

        if(flag){
            wait = wait+1;
            if(wait == 30){
                flag = 0;
                wait = 0;
                impulse_d = 0;
            }
        }
        if(impulse_d == 0){
            /*printf("nada\n");*/
            buff[1] = '0';
        }
        if(impulse_d == 1){
            /*printf("izquierda\n");*/
            buff[1] = 'L';
        }
        if(impulse_d == 2){
            /*printf("derecha\n");*/
            buff[1] = 'R';
        }


        printf("%s\n",buff);

        nanosleep(&tim, &tim2);

        archivo1 = fopen("tmp.txt","w");

        if(archivo1 == NULL){
            printf("No existe el archivo");
            return EXIT_FAILURE;
        }

        fwrite(buff,sizeof(char),sizeof(buff),archivo1);

        fclose(archivo1);

    }

    return EXIT_SUCCESS;

}
