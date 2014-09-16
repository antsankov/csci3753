 #include <unistd.h> 
  
int main () {
 
        setreuid (1337, 1337);
        system ("/bin/sh");
 
        return 0;
}