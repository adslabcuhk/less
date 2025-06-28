#ifndef __PROGRESS_BAR__
#define __PROGRESS_BAR__

#include <iostream>
#include <unistd.h>
#include <string>

#include "include.hh"

/**
 * @brief A simple progress bar for terminal
 * @param total The total number of iterations
 * @param width The width of the progress bar
 */

class ProgressBar
{
public:
    ProgressBar(unsigned long long total, int width = 50) : total_(total), width_(width), current_(0)
    {
        is_terminal_ = isatty(fileno(stdout));
    }

    void update(unsigned long long int current)
    {
        current_ = current;
        print();
    }

    void increment()
    {
        current_++;
        print();
    }

private:
    void print(); // print the progress bar

    unsigned long long int total_; // total number of iterations
    int width_; // width of the progress bar
    unsigned long long int current_; // current iteration
    bool is_terminal_; // check if the output is terminal
};

#endif // #ifndef __PROGRESS_BAR__