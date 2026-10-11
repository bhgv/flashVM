struct X { X() {try {throw 1;}catch(...){throw;}} };
int main() {try {X x;}catch(int){}return 0;}
