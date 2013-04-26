
 #ifndef KILNRUN_H
 #define KILNRUN_H

enum Speed       
{
    fast=0,       
    medium = 1,     
    slow = 2,
    aneal_slow = 3,
    aneal_medium = 4
};


class KilnRun {
  
   static const int coneMaxNdx = 29;// after this index, the coneNDX is reset to 0
   static const int maxHoldHrs = 9; // reset holdHrs to 0 if its incrimited past this by button push
    
  public:
  
 
    KilnRun(); //int8_t SCLK, int8_t CS, int8_t MISO);

    void candlePressed();
    void conePressed();
    void holdPressed();
    void speedPressed();
    void startPressed();
    void clearPressed();

  /*double readInternal(void);
  double readInternalF(void);
  double readCelsius(void);
  double readFarenheit(void);
  uint8_t readError();//*/

 private:
 
  int coneNDX; // selected temperature target
  int holdHrs; // hold the kiln at the target temperature for this many hours
  Speed speedMode;
  //int8_t sclk, miso, cs;
  //uint32_t spiread32(void);
};

 #endif 