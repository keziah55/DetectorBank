// Compile with g++ -I../src slidingbuffertest.cpp

#include <cstddef>
#include <iostream>

#include "slidingbuffer.h"

using namespace std;


class intsProducer : public slidingbuffer::SegmentProducer<int>
{
private:
    static constexpr size_t maxindex = 65;
    static int invoc;
public:
    size_t generate(int seg[],
                    const size_t idx,
                    const size_t size) {
        cout << "\nGenerating segment " << invoc 
             << " (" << size << " ints, org=" << idx << ") at " << seg;
        
        size_t i { 0 };
        while (i < size && (i + idx) < maxindex) {
            seg[i] = 1000*invoc + i;
            ++i;
        }
        
        ++invoc;

        cout << ". Generated " << i << "\n\t    ...";
        return i;
    }
    
    bool more(void) { return invoc < 7; }
};


int intsProducer::invoc = 1;

int main()
{
    intsProducer ip;
    // SlidingBuffer with 4 segments each 6 long.
    slidingbuffer::SlidingBuffer<int,
                                 slidingbuffer::Segment<int, intsProducer>,
                                 intsProducer> sb(ip, 4, 6);
    
    
    for (int i { 0 }; i < 80; i++) {
        //int h { 0 };
        for (int h { 0 }; h > -24; h-=8) {
            cout << "sb[" << i << " + ("<< h << ")] = ";
            try {
                cout << sb[i+h] << "\t";
            } catch (slidingbuffer::IndexOutOfRangeException e) {
                cout << "<EoD>(" << e.index << ") \t";
            } catch (slidingbuffer::SlidingBufferException e) {
                cout << "<OOR>(" << e.index << ") \t";
            }
        }
        cout << endl;
    }
}
