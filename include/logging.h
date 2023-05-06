#ifndef LOGGING_H
#define LOGGING_H

class Logging{

    private: 
        // should probably be a string
        const char * redtext(const char * txt); 
        const char * greentext(const char * txt); 
    public: 
        void log(const char * log_message); 
        void log_err(const char * error_message); 
        void log_info(const char * info_message);
        void print(const char * print_message); 
};

#endif
