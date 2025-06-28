#include "progressBar.hh"

void ProgressBar::print()
{
    float progress = static_cast<float>(current_) / total_;
    int pos = static_cast<int>(width_ * progress);

    if (is_terminal_)
    {
        std::cout << "\r[";
        for (int i = 0; i < width_; ++i)
        {
            if (i < pos)
                std::cout << "=";
            else if (i == pos)
                std::cout << ">";
            else
                std::cout << " ";
        }
        std::cout << "] " << int(progress * 100.0) << "% "
                  << "(" << current_ << "/" << total_ << ")"
                  << std::flush;

        if (current_ == total_)
            std::cout << std::endl;
    }
    else
    {
        if (current_ == total_)
        {
            std::cout << "[";
            for (int i = 0; i < width_; ++i)
                std::cout << "=";
            std::cout << "] 100% "
                      << "(" << total_ << "/" << total_ << ")"
                      << std::endl;
        }
    }
}