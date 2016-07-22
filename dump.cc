/*
  The MIT License (MIT)
 
  Copyright (c) 2011-2016 Broad Institute, Aiden Lab
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <streambuf>
#include <vector>
#include "zlib.h"
using namespace std;

#define CHUNK 16384
/*
  Quick dump: fast C++ implementation of dump. Not as fully featured as the
  Java version. Reads the .hic file, finds the appropriate matrix and slice
  of data, and outputs as text in sparse upper triangular format.

  Currently only supporting matrices.

  Usage: juicebox-quick-dump <NONE/VC/VC_SQRT/KR> <hicFile(s)> <chr1>[:x1:x2] <chr2>[:y1:y2] <BP/FRAG> <binsize> 
 */
// this is for creating a stream from a byte array for ease of use
struct membuf : std::streambuf
{
  membuf(char* begin, char* end) {
    this->setg(begin, begin, end);
  }
};

struct indexEntry {
  int size;
  long position;
};

struct contactRecord {
  int binX;
  int binY;
  float counts;
};

int version;
map <int, indexEntry> blockMap;

bool readMagicString(ifstream& fin) {
  string str;
  getline(fin, str, '\0' );
  return str[0]=='H' && str[1]=='I' && str[2]=='C';
}

long readHeader(ifstream& fin, string chr1, string chr2, int &c1pos1, int &c1pos2, int &c2pos1, int &c2pos2, int &chr1ind, int &chr2ind) {
  if (!readMagicString(fin)) {
    cerr << "Hi-C magic string is missing, does not appear to be a hic file" << endl;
    exit(1);
  }

  fin.read((char*)&version, sizeof(int));
  if (version < 6) {
    cerr << "Version " << version << " no longer supported" << endl;
    exit(1);
  }
  long master;
  fin.read((char*)&master, sizeof(long));
  string genome;
  getline(fin, genome, '\0' );
  int nattributes;
  fin.read((char*)&nattributes, sizeof(int));
  // reading and ignoring attribute-value dictionary
  for (int i=0; i<nattributes; i++) {
    string key, value;
    getline(fin, key, '\0');
    getline(fin, value, '\0');
  }
  int nChrs;
  fin.read((char*)&nChrs, sizeof(int));
  // chromosome map for finding matrix
  bool found1 = false;
  bool found2 = false;
  for (int i=0; i<nChrs; i++) {
    string name;
    int length;
    getline(fin, name, '\0');
    fin.read((char*)&length, sizeof(int));
    if (name==chr1) {
      found1=true;
      chr1ind = i;
      if (c1pos1 == -100) {
	c1pos1 = 0;
	c1pos2 = length;
      }
    }
    if (name==chr2) {
      found2=true;
      chr2ind = i;
      if (c2pos1 == -100) {
	c2pos1 = 0;
	c2pos2 = length;
      }
    }
  }
  if (!found1 || !found2) {
    cerr << "One of the chromosomes wasn't found in the file. Check that the chromosome name matches the genome." << endl;
    exit(1);
  }
  return master;
}

void readFooter(ifstream& fin, long master, int c1, int c2, string norm, string unit, int resolution, int &mySize, long &myFilePos, int &c1NormSizeInBytes, long &c1NormFilePosition, int &c2NormSizeInBytes, long &c2NormFilePosition) {
  fin.seekg(master, ios::beg);
  int nBytes;
  fin.read((char*)&nBytes, sizeof(int));

  stringstream ss;
  ss << c1 << "_" << c2;
  string key = ss.str();
  
  int nEntries;
  fin.read((char*)&nEntries, sizeof(int));
  bool found = false;
  for (int i=0; i<nEntries; i++) {
    string str;
    getline(fin, str, '\0');
    long fpos;
    fin.read((char*)&fpos, sizeof(long));
    int sizeinbytes;
    fin.read((char*)&sizeinbytes, sizeof(int));
    if (str == key) {
      myFilePos = fpos;
      mySize = sizeinbytes;
      found=true;
    }
  }
  if (!found) {
    cerr << "File doesn't have the given chr_chr map" << endl;
    exit(1);
  }
  
  if (norm=="NONE") return; // no need to read norm vector index
 
  // read in and ignore expected value maps; don't store; reading these to 
  // get to norm vector index
  int nExpectedValues;
  fin.read((char*)&nExpectedValues, sizeof(int));
  for (int i=0; i<nExpectedValues; i++) {
    string str;
    getline(fin, str, '\0'); //unit
    int binSize;
    fin.read((char*)&binSize, sizeof(int));

    int nValues;
    fin.read((char*)&nValues, sizeof(int));
    for (int j=0; j<nValues; j++) {
      double v;
      fin.read((char*)&v, sizeof(double));
    }

    int nNormalizationFactors;
    fin.read((char*)&nNormalizationFactors, sizeof(int));
    for (int j=0; j<nNormalizationFactors; j++) {
      int chrIdx;
      fin.read((char*)&chrIdx, sizeof(int));
      double v;
      fin.read((char*)&v, sizeof(double));
    }
  }
  fin.read((char*)&nExpectedValues, sizeof(int));
  for (int i=0; i<nExpectedValues; i++) {
    string str;
    getline(fin, str, '\0'); //typeString
    getline(fin, str, '\0'); //unit
    int binSize;
    fin.read((char*)&binSize, sizeof(int));

    int nValues;
    fin.read((char*)&nValues, sizeof(int));
    for (int j=0; j<nValues; j++) {
      double v;
      fin.read((char*)&v, sizeof(double));
    }
    int nNormalizationFactors;
    fin.read((char*)&nNormalizationFactors, sizeof(int));
    for (int j=0; j<nNormalizationFactors; j++) {
      int chrIdx;
      fin.read((char*)&chrIdx, sizeof(int));
      double v;
      fin.read((char*)&v, sizeof(double));
    }
  }
  // Index of normalization vectors
  fin.read((char*)&nEntries, sizeof(int));
  bool found1 = false;
  bool found2 = false;
  for (int i = 0; i < nEntries; i++) {
    string normtype;
    getline(fin, normtype, '\0'); //normalization type
    int chrIdx;
    fin.read((char*)&chrIdx, sizeof(int));
    string unit1;
    getline(fin, unit1, '\0'); //unit
    int resolution1;
    fin.read((char*)&resolution1, sizeof(int));
    long filePosition;
    fin.read((char*)&filePosition, sizeof(long));
    int sizeInBytes;
    fin.read((char*)&sizeInBytes, sizeof(int));
    if (chrIdx == c1 && normtype == norm && unit1 == unit && resolution1 == resolution) {
      c1NormFilePosition=filePosition;
      c1NormSizeInBytes=sizeInBytes;
      found1 = true;
    }
    if (chrIdx == c2 && normtype == norm && unit1 == unit && resolution1 == resolution) {
      c2NormFilePosition=filePosition;
      c2NormSizeInBytes=sizeInBytes;
      found2 = true;
    }
  }
  if (!found1 || !found2) {
    cerr << "File did not contain " << norm << " normalization vectors for one or both chromosomes at " << resolution << " " << unit << endl;
    exit(1);
  }
}

bool readMatrixZoomData(ifstream& fin, string myunit, int mybinsize, int &myBlockBinCount, int &myBlockColumnCount) {
  string unit;
  getline(fin, unit, '\0' ); // unit
  int tmp;
  fin.read((char*)&tmp, sizeof(int)); // Old "zoom" index -- not used
  float tmp2;
  fin.read((char*)&tmp2, sizeof(float)); // sumCounts
  fin.read((char*)&tmp2, sizeof(float)); // occupiedCellCount
  fin.read((char*)&tmp2, sizeof(float)); // stdDev
  fin.read((char*)&tmp2, sizeof(float)); // percent95
  int binSize;
  fin.read((char*)&binSize, sizeof(int));
  int blockBinCount;
  fin.read((char*)&blockBinCount, sizeof(int));
  int blockColumnCount;
  fin.read((char*)&blockColumnCount, sizeof(int));
  
  bool storeBlockData = false;
  if (myunit==unit && mybinsize==binSize) {
    myBlockBinCount = blockBinCount;
    myBlockColumnCount = blockColumnCount;
    storeBlockData = true;
  }
//MatrixZoomData zd = new MatrixZoomData(chr1, chr2, zoom, blockBinCount, blockColumnCount, chr1Sites, chr2Sites, this);
  
  int nBlocks;
  fin.read((char*)&nBlocks, sizeof(int));
  //HashMap<Integer, Preprocessor.IndexEntry> blockIndex = new HashMap<Integer, Preprocessor.IndexEntry>(nBlocks);

  for (int b = 0; b < nBlocks; b++) {
    int blockNumber;
    fin.read((char*)&blockNumber, sizeof(int));
    long filePosition;
    fin.read((char*)&filePosition, sizeof(long));
    int blockSizeInBytes;
    fin.read((char*)&blockSizeInBytes, sizeof(int));
    indexEntry entry;
    entry.size = blockSizeInBytes;
    entry.position = filePosition;
    if (storeBlockData) blockMap[blockNumber] = entry;
  }
  return storeBlockData;
}

void readMatrix(ifstream& fin, int myFilePosition, string unit, int resolution, int &myBlockBinCount, int &myBlockColumnCount) {
  fin.seekg(myFilePosition, ios::beg);
  int c1,c2;
  fin.read((char*)&c1, sizeof(int)); //chr1
  fin.read((char*)&c2, sizeof(int)); //chr2
  int nRes;
  fin.read((char*)&nRes, sizeof(int));
  int i=0;
  bool found=false;
  while (i<nRes && !found) {
    found = readMatrixZoomData(fin, unit, resolution, myBlockBinCount, myBlockColumnCount);
    i++;
  }
  if (!found) {
    cerr << "Error finding block data" << endl;
    exit(1);
  }
}
// need blockbincount, blockcolumncount.
set<int> getBlockNumbersForRegionFromBinPosition(int* regionIndices, int blockBinCount, int blockColumnCount, bool intra) {
   int col1 = regionIndices[0] / blockBinCount;
   int col2 = (regionIndices[1] + 1) / blockBinCount;
   int row1 = regionIndices[2] / blockBinCount;
   int row2 = (regionIndices[3] + 1) / blockBinCount;
   
   set<int> blocksSet;
   // first check the upper triangular matrix
   for (int r = row1; r <= row2; r++) {
     for (int c = col1; c <= col2; c++) {
       int blockNumber = r * blockColumnCount + c;
       blocksSet.insert(blockNumber);
     }
   }
   // check region part that overlaps with lower left triangle
   // but only if intrachromosomal
   if (intra) {
     for (int r = col1; r <= col2; r++) {
       for (int c = row1; c <= row2; c++) {
	 int blockNumber = r * blockColumnCount + c;
	 blocksSet.insert(blockNumber);
       }
     }
   }

   return blocksSet;
}


vector<contactRecord> readBlock(ifstream& fin, int blockNumber) {
  indexEntry idx = blockMap[blockNumber];
  if (idx.size == 0) {
    vector<contactRecord> v;
    return v;
  }
  char compressedBytes[idx.size];
  char* uncompressedBytes = new char[idx.size*10]; //biggest seen so far is 3
  fin.seekg(idx.position, ios::beg);
  fin.read(compressedBytes, idx.size);
  // Decompress the block
  // zlib struct
  z_stream infstream;
  infstream.zalloc = Z_NULL;
  infstream.zfree = Z_NULL;
  infstream.opaque = Z_NULL;
  infstream.avail_in = (uInt)(idx.size); // size of input
  infstream.next_in = (Bytef *)compressedBytes; // input char array
  infstream.avail_out = (uInt)idx.size*10; // size of output
  infstream.next_out = (Bytef *)uncompressedBytes; // output char array
  // the actual decompression work.
  inflateInit(&infstream);
  inflate(&infstream, Z_NO_FLUSH);
  inflateEnd(&infstream);
  int uncompressedSize=infstream.total_out;
  //cerr << "compression ratio: " << uncompressedSize*1.0/idx.size << endl;
  // create stream from buffer for ease of use
  membuf sbuf(uncompressedBytes, uncompressedBytes + uncompressedSize);
  istream bufferin(&sbuf);
  int nRecords;
  bufferin.read((char*)&nRecords, sizeof(int));
  vector<contactRecord> v(nRecords);
  if (version < 7) {
    for (int i = 0; i < nRecords; i++) {
      int binX, binY;
      bufferin.read((char*)&binX, sizeof(int));
      bufferin.read((char*)&binY, sizeof(int));
      float counts;
      bufferin.read((char*)&counts, sizeof(float));
      contactRecord record;
      record.binX = binX;
      record.binY = binY;
      record.counts = counts;
      v[i] = record;
    }
  } 
  else {
    int binXOffset, binYOffset;
    bufferin.read((char*)&binXOffset, sizeof(int));
    bufferin.read((char*)&binYOffset, sizeof(int));
    char useShort;
    bufferin.read((char*)&useShort, sizeof(char));
    char type;
    bufferin.read((char*)&type, sizeof(char));
    int index=0;
    if (type == 1) {
      // List-of-rows representation
      short rowCount;
      bufferin.read((char*)&rowCount, sizeof(short));
      for (int i = 0; i < rowCount; i++) {
	short y;
	bufferin.read((char*)&y, sizeof(short));
	int binY = y + binYOffset;
	short colCount;
	bufferin.read((char*)&colCount, sizeof(short));
	for (int j = 0; j < colCount; j++) {
	  short x;
	  bufferin.read((char*)&x, sizeof(short));
	  int binX = binXOffset + x;
	  float counts;
	  if (useShort == 0) { // yes this is opposite of usual
	    short c;
	    bufferin.read((char*)&c, sizeof(short));
	    counts = c;
	  } 
	  else {
	    bufferin.read((char*)&counts, sizeof(float));
	  }
	  contactRecord record;
	  record.binX = binX;
	  record.binY = binY;
	  record.counts = counts;
	  v[index]=record;
	  index++;
	}
      }
    }
    else if (type == 2) { // have yet to find test file where this is true
      int nPts;
      bufferin.read((char*)&nPts, sizeof(int));
      short w;
      bufferin.read((char*)&w, sizeof(short));

      for (int i = 0; i < nPts; i++) {
	//int idx = (p.y - binOffset2) * w + (p.x - binOffset1);
	int row = i / w;
	int col = i - row * w;
	int bin1 = binXOffset + col;
	int bin2 = binYOffset + row;

	float counts;
	if (useShort == 0) { // yes this is opposite of the usual
	  short c;
	  bufferin.read((char*)&c, sizeof(short));
	  if (c != -32768) {
	    contactRecord record;
	    record.binX = bin1;
	    record.binY = bin2;
	    record.counts = counts;
	    v[index]=record;
	    index++;
	  }
	} 
	else {
	  bufferin.read((char*)&counts, sizeof(float));
	  if (counts != 0x7fc00000) { // not sure this works
	    //	  if (!Float.isNaN(counts)) {
	    contactRecord record;
	    record.binX = bin1;
	    record.binY = bin2;
	    record.counts = counts;
	    v[index]=record;
	    index++;
	  }
	}
      }
    }
  }
  delete uncompressedBytes;
  return v;
}

vector<double> readNormalizationVector(ifstream& fin, int size, long position) {
  char buffer[size];
  fin.seekg(position, ios::beg);
  fin.read(buffer, size);
  membuf sbuf(buffer, buffer + size);
  istream bufferin(&sbuf);
  int nValues;
  bufferin.read((char*)&nValues, sizeof(int));
  vector<double> values(nValues);
  //  bool allNaN = true;

  for (int i = 0; i < nValues; i++) {
    double d;
    bufferin.read((char*)&d, sizeof(double));
    values[i] = d;
    /* if (!Double.isNaN(values[i])) {
      allNaN = false;
      }*/
  }
  //  if (allNaN) return null;
  return values;
}

int main(int argc, char *argv[])
{
  if (argc != 7) {
    cerr << "Not enough arguments" << endl;
    cerr << "Usage: juicebox-quick-dump <NONE/VC/VC_SQRT/KR> <hicFile(s)> <chr1>[:x1:x2] <chr2>[:y1:y2] <BP/FRAG> <binsize>" << endl;
    exit(1);
  }

  string norm=argv[1];
  if (!(norm=="NONE"||norm=="VC"||norm=="VC_SQRT"||norm=="KR")) {
    cerr << "Norm specified incorrectly, must be one of <NONE/VC/VC_SQRT/KR>" << endl; 
    cerr << "Usage: juicebox-quick-dump <NONE/VC/VC_SQRT/KR> <hicFile(s)> <chr1>[:x1:x2] <chr2>[:y1:y2] <BP/FRAG> <binsize>" << endl;
    exit(1);
  }

  string unit=argv[5];
  if (!(unit=="BP"||unit=="FRAG")) {
    cerr << "Norm specified incorrectly, must be one of <BP/FRAG>" << endl; 
    cerr << "Usage: juicebox-quick-dump <NONE/VC/VC_SQRT/KR> <hicFile(s)> <chr1>[:x1:x2] <chr2>[:y1:y2] <BP/FRAG> <binsize>" << endl;
    exit(1);
  }

  string size=argv[6];
  int binsize=stoi(size);

  ifstream fin(argv[2], fstream::in);
  if (!fin) {
    cerr << "File " << argv[2] << " cannot be opened for reading" << endl;
    exit(1);
  }

  string ch=argv[3];
  stringstream ss(ch);
  string chr1, chr2, x, y;
  int c1pos1=-100, c1pos2=-100, c2pos1=-100, c2pos2=-100;
  getline(ss, chr1, ':');
  if (getline(ss, x, ':') && getline(ss, y, ':')) {
    c1pos1 = stoi(x);
    c1pos2 = stoi(y);
  }

  ch=argv[4];
  stringstream ss1(ch);
  getline(ss1, chr2, ':');
  if (getline(ss1, x, ':') && getline(ss1, y, ':')) {
    c2pos1 = stoi(x);
    c2pos2 = stoi(y);
  }  
  int chr1ind, chr2ind;
  long master = readHeader(fin, chr1, chr2, c1pos1, c1pos2, c2pos1, c2pos2, chr1ind, chr2ind);
  
  int c1=min(chr1ind,chr2ind);
  int c2=max(chr1ind,chr2ind);
  int origRegionIndices[4]; // as given by user
  int regionIndices[4]; // used to find the blocks we need to access
  // reverse order if necessary
  if (chr1ind > chr2ind) {
    origRegionIndices[0] = c2pos1;
    origRegionIndices[1] = c2pos2;
    origRegionIndices[2] = c1pos1;
    origRegionIndices[3] = c1pos2;
    regionIndices[0] = c2pos1 / binsize;
    regionIndices[1] = c2pos2 / binsize;
    regionIndices[2] = c1pos1 / binsize;
    regionIndices[3] = c1pos2 / binsize;
  }
  else {
    origRegionIndices[0] = c1pos1;
    origRegionIndices[1] = c1pos2;
    origRegionIndices[2] = c2pos1;
    origRegionIndices[3] = c2pos2;
    regionIndices[0] = c1pos1 / binsize;
    regionIndices[1] = c1pos2 / binsize;
    regionIndices[2] = c2pos1 / binsize;
    regionIndices[3] = c2pos2 / binsize;
  }

  int mySize, c1NormSizeInBytes, c2NormSizeInBytes;
  long myFilePos, c1NormFilePosition, c2NormFilePosition;

  // readFooter will assign the above variables
  readFooter(fin, master, c1, c2, norm, unit, binsize, mySize, myFilePos, c1NormSizeInBytes, c1NormFilePosition, c2NormSizeInBytes, c2NormFilePosition);

  vector<double> c1Norm;
  vector<double> c2Norm;

  if (norm != "NONE") {
    c1Norm = readNormalizationVector(fin, c1NormSizeInBytes, c1NormFilePosition);
    c2Norm = readNormalizationVector(fin, c2NormSizeInBytes, c2NormFilePosition);
  }
  int blockBinCount, blockColumnCount;
  // readMatrix will assign blockBinCount and blockColumnCount
  readMatrix(fin, myFilePos, unit, binsize, blockBinCount, blockColumnCount); 

  set<int> blockNumbers = getBlockNumbersForRegionFromBinPosition(regionIndices, blockBinCount, blockColumnCount, c1==c2); 

  // getBlockIndices
  vector<contactRecord> records;
  for (set<int>::iterator it=blockNumbers.begin(); it!=blockNumbers.end(); ++it) {
    // get contacts in this block
    records = readBlock(fin, *it);
    for (vector<contactRecord>::iterator it2=records.begin(); it2!=records.end(); ++it2) {
      contactRecord rec = *it2;
      
      int xActual = rec.binX * binsize;
      int yActual = rec.binY * binsize;
      float counts = rec.counts;
      if (norm != "NONE") {
	counts = counts / (c1Norm[rec.binX] * c2Norm[rec.binY]);
      }
//      cout << xActual << " " << yActual << " " << counts << endl;
      if ((xActual >= origRegionIndices[0] && xActual <= origRegionIndices[1] &&
	   yActual >= origRegionIndices[2] && yActual <= origRegionIndices[3]) ||
	  // or check regions that overlap with lower left
	  ((c1==c2) && yActual >= origRegionIndices[0] && yActual <= origRegionIndices[1] && xActual >= origRegionIndices[2] && xActual <= origRegionIndices[3])) {
	printf("%d\t%d\t%.14g\n", xActual, yActual, counts);
      }
    }
  }
}

