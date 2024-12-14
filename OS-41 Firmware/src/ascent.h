//Functions for calculating ascent rate

//NOTE: ***all functions assume time difference between altitude (GPS update rate) is 1 second***

//ANOTHER NOTE: ***ALL FUNCTIONS ASSUME that altitude_previous and altitude are changed. This is the responsability of the caller

uint16_t altitude_previous = 0;

int verticalVel[4] = {0,0,0,0}; //Buffer to store velocities for averaging

int currentAscentRate = 0;

float IRAM_ATTR average (int * array, int len){
  long sum = 0L; 
  for (int i = 0 ; i < len ; i++)
    sum += array [i];
  return  ((float) sum) / len; 
}

void IRAM_ATTR updateRates(){
    currentAscentRate = altitude-altitude_previous;

    //Update rolling averaging buffer (there needs to be a better way of doing this)
    verticalVel[0] = verticalVel[1];
    verticalVel[1] = verticalVel[2];
    verticalVel[2] = verticalVel[3];
    verticalVel[3] = currentAscentRate;

    //Average array and multiply by 100 (to suit sondehub), then write it to the current value of ascentRate
    ascentRate = average(verticalVel,4)*100;
}