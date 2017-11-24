//#include <iostream>
#include "defragmenter.h"
#include "mynew.h"
#include "DefragRunner.h"
#include "QuadraticProbing.h"
#include "BinaryHeap.h"
//using namespace std;


Defragmenter::Defragmenter(DiskDrive *diskDrive) {

    DiskBlock *nf = NULL;
    QuadraticHashTable<DiskBlock*>* table = new QuadraticHashTable<DiskBlock*>(nf, 6000);
//    table = new QuadraticHashTable<DiskBlock*>(nf, 6000);
    QuadraticHashTable<unsigned>* hashIndex;
    hashIndex = new QuadraticHashTable<unsigned>(-1, 20000);

    unsigned empty = 200000;
    BinaryHeap <int>* heap = new BinaryHeap <int> (empty + 1);
//    heap = new BinaryHeap <int> (empty + 1);

    unsigned count = 0;
    unsigned cap = diskDrive->getCapacity() - 1;
    float l = empty/1.5;
    for (; cap >= 2 && count <= l; cap--) {
        if (diskDrive->FAT[cap] == false) { // unused
            heap->insert(-cap);
            count++;
        }
    }
    unsigned minimum = (diskDrive->getCapacity() + cap) / 2;
    unsigned index = 2;
    DiskBlock* head = NULL;
    DiskBlock* move = NULL;
    unsigned currentLocation = 0;
    int numFile = diskDrive->getNumFiles();
    for (int i = 0; i < numFile ; i++) {
        unsigned blockID = diskDrive->directory[i].getFirstBlockID();
        if (blockID == diskDrive->directory[i].getFirstBlockID()) { // first
            diskDrive->directory[i].setFirstBlockID(index); //first file starts on disk block #2
        }
        while (blockID != 1) {
            if (blockID < index) { // read from table
                head = table->find(blockID); //return item that matches blockID
                if(!head) {
                    currentLocation = hashIndex->find(blockID);
                    unsigned x = -1;
                    while (hashIndex->find(currentLocation) != x) {
                        hashIndex->remove(blockID);
                        blockID = currentLocation;
                        currentLocation = hashIndex->find(blockID);
                    }
                    head = diskDrive->readDiskBlock(currentLocation);
                    if (currentLocation != index) { //current Position does not equal index
                        diskDrive->FAT[currentLocation] = false; //empty
                        if (currentLocation > minimum && heap->getSize() < empty) { //extra empty values
                            heap->insert(-currentLocation);
                        }
                    }
                    hashIndex->remove(blockID);
                }
                else {
                    table->remove(blockID); //removes blockID
                }
                /*
                if (head) {
                    table->remove(blockID); //removes blockID
                }
                else { // read from disk
                    currentLocation = hashIndex->find(blockID);
                    unsigned x = -1;
                    while (hashIndex->find(currentLocation) != x) {
                        hashIndex->remove(blockID);
                        blockID = currentLocation;
                        currentLocation = hashIndex->find(blockID);
                    }
                    head = diskDrive->readDiskBlock(currentLocation);
                    if (currentLocation != index) { //current Position does not equal index
                        diskDrive->FAT[currentLocation] = false; //empty
                        if (currentLocation > minimum && heap->getSize() < empty) { //extra empty values
                            heap->insert(-currentLocation);
                        }
                    }
                    hashIndex->remove(blockID);
                }*/
            }
            else { // read from disk
                head = diskDrive->readDiskBlock(blockID);
                if (blockID != index) {
                    diskDrive->FAT[blockID] = false;
                    if (currentLocation > minimum && heap->getSize() < empty) {
                        heap->insert(-blockID);
                    }
                }
            }
            if (blockID != index) { // need to write to index
                if (diskDrive->FAT[index]) { // if FAT = true = used, move it to table
                    move = diskDrive->readDiskBlock(index);
                    if (table->needRehash()) {
                        cap = -heap->findMin();
                        heap->deleteMin();
                        diskDrive->writeDiskBlock(move,cap);
                        hashIndex->insert(index,cap);
                        diskDrive->FAT[cap] = true;
                        delete move;
                    }
                    else { // put it in hashIndex
                        table->insert(index, move);
                    }
                }
                blockID = head->getNext();
                if (blockID != 1) {
                    head->setNext(index + 1);
                }
                diskDrive->writeDiskBlock(head, index);
                diskDrive->FAT[index] = true;
                delete head;
            }
            else { // blockID == index
                blockID = head->getNext();
            }
            index++;
        }
    }
    delete table;
    delete hashIndex;
    delete heap;
}
