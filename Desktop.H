// Desktop.H

class Desktop {
  const char* name_;
  int number_;
  static Desktop* current_;
public:
  static Desktop* first;
  Desktop* next;
  const char* name() const {return name_;}
  void name(const char*);
  int number() const {return number_;}
  static Desktop* current() {return current_;}
  static Desktop* number(int, int create = 0);
  static void current(Desktop*);
  static int available_number();
  static int max_number();
  Desktop(const char*, int);
  ~Desktop();
  int junk; // for temporary storage by menu builder
};

