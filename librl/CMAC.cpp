#include "CMAC.hh"
#include "Tiles.hh"

//CMAC constructor
CMAC::CMAC(int _numFloatInputs, int _numIntInputs, int _memorySize, 
	   double _alpha, int _numTilings, int _tableDimensionality)
  :  alpha(_alpha), 
     numFloatInputs(_numFloatInputs),
     numIntInputs(_numIntInputs),
     memorySize(_memorySize),
     numTilings(_numTilings),
     tableDimensionality(_tableDimensionality)
{
  //Allocate input, tile, and cmac arrays
  floatInputs = new float[numFloatInputs];
  intInputs = new int[numIntInputs];
  tiles = new int[numTilings];
  u = new double[memorySize];
  

  //Initialize input, tile, and CMAC arrays
  for(int i=0; i<numFloatInputs; i++){
    floatInputs[i] = 0;
  }
  for(int i=0; i<numIntInputs; i++){
    intInputs[i] = 0;
  }
  for(int i=0; i<numTilings; i++){
    tiles[i] = 0;
  }
  for(int i=0; i<memorySize; i++){
    u[i] = 0;
  }
}

//Predict target for current input vector
double CMAC::predict()
{
  //Populate tiles array with tile indices
  GetTiles(tiles, numTilings, memorySize, floatInputs, numFloatInputs, intInputs, numIntInputs);

  //Calculate the sum of all indexed tiles
  double sum = 0;
  for(int i=0; i<numTilings; i++){
    sum += u[tiles[i]];
  }
  
  //Return the sum
  return sum;

}

//Predict target for current input vector
double CMAC::predictOR()
{

  //Number of distinct tables
  int numDistinctTables = numFloatInputs / tableDimensionality; 

  //Populate tiles array with tile indices
  for(int i=0; i<numDistinctTables; i++){
    intInputs[1] = i;
    int begin = (numTilings / numDistinctTables) * i;
    GetTiles(&tiles[begin], (numTilings / numDistinctTables), memorySize, &floatInputs[i], tableDimensionality, intInputs, numIntInputs);
  }
  
  //Calculate the sum of all indexed tiles
  double sum = 0;
  for(int i=0; i<numTilings; i++){
    sum += u[tiles[i]];
  }
  
  //Return the sum
  return sum;
  
}

//Train CMAC on current input towards given target
void CMAC::update(double target)
{
  //Populate tiles array with tile indices
  GetTiles(tiles, numTilings, memorySize, floatInputs, numFloatInputs, intInputs, numIntInputs);
  
  //Calculate the sum of all indexed tiles
  double pred = 0;
  for(int i=0; i<numTilings; i++){
    pred += u[tiles[i]];
  }
  
  //Train CMAC memory cells
  for(int i=0; i<numTilings; i++){
    u[tiles[i]] += ((alpha / numTilings) * (target - pred));
  }
  
}

//Train CMAC on current input towards given target
void CMAC::updateOR(double target)
{

  //Number of distinct tables
  int numDistinctTables = numFloatInputs / tableDimensionality; 

  //Populate tiles array with tile indices
  for(int i=0; i<numDistinctTables; i++){
    intInputs[1] = i;
    int begin = (numTilings / numDistinctTables) * i;
    GetTiles(&tiles[begin], (numTilings / numDistinctTables), memorySize, &floatInputs[i], tableDimensionality, intInputs, numIntInputs);
  }
  
  //Calculate the sum of all indexed tiles
  double pred = 0;
  for(int i=0; i<numTilings; i++){
    pred += u[tiles[i]];
  }
  
  //Train CMAC memory cells
  for(int i=0; i<numTilings; i++){
    u[tiles[i]] += ((alpha / numTilings) * (target - pred));
  }
  
}
