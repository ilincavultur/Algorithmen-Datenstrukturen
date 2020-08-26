#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>

template <typename Key, size_t N = 7>
class ADS_set {
public:
  class Iterator;
  using value_type = Key;
  using key_type = Key;
  using reference = key_type &;
  using const_reference = const key_type &;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using iterator = Iterator;
  using const_iterator = Iterator;
  using key_compare = std::less<key_type>;   // B+-Tree
  using key_equal = std::equal_to<key_type>; // Hashing
  using hasher = std::hash<key_type>;        // Hashing

private:
  enum class Mode {free, used, end};
  struct element {
    key_type key;
    Mode mode {Mode::free};
    element *next_el {nullptr};
  };
  element *table{nullptr};
  float max_lf {0.7};
  float keller {0.1628};
  size_type table_size{0}, curr_size{0}, last_element{0}; //last_element points to the last free element of the table (actually an element AFTER that) BUT only until it s in the keller
  // after the keller is full, it doesn't necessarily point to the last FREE element, it just points to the last element which can be USED
  size_type h(const key_type &key) const { return hasher{}(key) % table_size; }
    

  element *insert_(const key_type &key);
  void rehash(size_type n);
  void reserve(size_type n);
  element *find_(const key_type &key) const;

public:
  ADS_set() {rehash(N);}
  ADS_set(std::initializer_list<key_type> ilist): ADS_set{}{     
              //ilist Constructur
      insert(ilist);
  }
  template<typename InputIt> ADS_set(InputIt first, InputIt last): ADS_set{}{   //Range-Consstructor with list of Inputs
    
    insert(first, last);
  }
  ADS_set(const ADS_set &other): ADS_set{} {
    reserve(other.curr_size);
    
    for (const auto &k: other) {
      insert_(k);
    }
    
  }

  ~ADS_set() { delete[] table; }

  ADS_set &operator=(const ADS_set &other){
    if (this == &other) return *this;
    ADS_set tmp{other};
    swap(tmp);
    return *this;
  }
  ADS_set &operator=(std::initializer_list<key_type> ilist){
    ADS_set tmp{ilist};
    swap(tmp);
    return *this;
  }

  size_type size() const { return curr_size; }
  bool empty() const { return curr_size == 0 ; }

  size_type count(const key_type &key) const { return !!find_(key); }
  iterator find(const key_type &key) const{
    if (auto p {find_(key)}) return iterator{p};
    return end();
  }
  
  void clear(){
    ADS_set tmp;
    swap(tmp);
  }
  void swap(ADS_set &other){
    using std::swap;
    swap(table, other.table);
    swap(table_size, other.table_size);
    swap(curr_size, other.curr_size);
    swap(max_lf, other.max_lf);
    swap(keller, other.keller);
    swap(last_element, other.last_element);
    
  }

 void insert(std::initializer_list<key_type> ilist){ 
                            //insert ilist
    insert(std::begin(ilist), std::end(ilist));
  
    
  }
  std::pair<iterator,bool> insert(const key_type &key){
    if (auto p {find_(key)}) return {iterator{p},false};
    reserve(curr_size+1);
    return {iterator{insert_(key)},true};
  }
  
  template<typename InputIt> void insert(InputIt first, InputIt last){             //insert Template
    
        for (auto it {first}; it != last; ++it){
          if (!count(*it)){
            reserve(curr_size+1);
            insert_(*it);
          }
        }
    
    
  }

  size_type erase(const key_type &key);

  const_iterator begin() const{ return const_iterator{table}; }
  const_iterator end() const{ return const_iterator{table+table_size+ static_cast<int>(table_size * keller)};}

  void dump(std::ostream &o = std::cerr) const;

  friend bool operator==(const ADS_set &lhs, const ADS_set &rhs){
    if (lhs.curr_size != rhs.curr_size) return false;
    for (const auto &k: rhs) if (!lhs.count(k)) return false;
    return true;
  }

  friend bool operator!=(const ADS_set &lhs, const ADS_set &rhs){
    return !(lhs == rhs);
  }
};

template <typename Key, size_t N>
typename ADS_set<Key,N>::element *ADS_set<Key,N>::insert_(const key_type &key){
    element *index = &table[h(key)];
    size_type idx{h(key)};
    if ( index->mode == Mode::end) throw std::runtime_error("asfasa");

    if ( index->mode == Mode::used){ //Kollision

        while (index->next_el != nullptr){
            
            index = index->next_el;
            
        }
      
        if (--last_element <= table_size){ //Keller voll; -- bc last_element points to the element AFTER the actual last element
          for (size_type i{table_size-1}; i >= 0; --i){
            if (table[i].mode == Mode::free){
              table[i].key = key;
              table[i].mode = Mode::used;
              index->next_el = &table[i];
              curr_size++;
              return table+i;
            }
          }
        }
        table[last_element].key = key;
        table[last_element].mode = Mode::used;
        index->next_el = &table[last_element];
        last_element--;
        curr_size++;
        return table+last_element+1; // bc you decremented last_element
    }

    if(table[idx].mode == Mode::free){
        table[idx].key = key;
        table[idx].mode = Mode::used;
        curr_size++;
        return table+idx;
    }

    return nullptr;
  }

template <typename Key, size_t N>
void ADS_set<Key,N>::rehash(size_type n){
    
    size_type old_table_sz {table_size};
    int old_kell {static_cast<int>(old_table_sz * keller)};
    element *old_table {table};

    size_type new_table_sz {std::max(N, std::max(n,size_type(curr_size / max_lf)))};
    int new_kell = static_cast<int>(new_table_sz * keller);
    element *new_table {new element[new_table_sz + new_kell + 1]};
    new_table[new_table_sz + new_kell].mode = Mode::end;
    
    table = new_table;
    table_size = new_table_sz;

    curr_size = 0; //number of elements inserted in the table at the moment

    for ( size_type idx{0}; idx < old_table_sz + old_kell ; ++idx ){
        if ( old_table[idx].mode == Mode::used ) insert_(old_table[idx].key);
    } 
    
    delete[] old_table;
    last_element = new_table_sz + new_kell ;
  }

template <typename Key, size_t N>
void ADS_set<Key,N>::reserve(size_type n){
    int kell = static_cast<int>(table_size * keller);
      if ( n > (table_size + kell) * max_lf ){
          size_type new_table_sz{table_size};
           do {
              new_table_sz = new_table_sz * 2 + 1;
              kell = static_cast<int>(new_table_sz * keller);
          } while (n > (new_table_sz + kell) * max_lf);
        rehash(new_table_sz);
      }
  }

template <typename Key, size_t N>
typename ADS_set<Key,N>::element *ADS_set<Key,N>::find_(const key_type &key) const{
    element *index = &table[h(key)];
    //size_type idx{h(key)};
    if (index->mode == Mode::end) {return nullptr;} 
      if (index->mode == Mode::used){
          if (key_equal{}(key, index->key)) {
              return index;
          }
          while (index->mode != Mode::end && index->next_el != nullptr ){
              index = index->next_el;
              if (key_equal{}(key, index->key)) return index; 
              
          }
          //if (table[idx].mode == Mode::end) return nullptr;
          
      }
    return nullptr;

  }

template <typename Key, size_t N>
void ADS_set<Key,N>::dump(std::ostream &o) const {
  o << "current size: " << curr_size << " table size: " << table_size << '\n';
      for (size_type idx{0}; idx < table_size + static_cast<int>(table_size*keller) + 1; ++idx){
        o << idx << ": ";
            switch (table[idx].mode) {
                case Mode::used:
                    o << table[idx].key;
                    break;
                case Mode::free:
                    o << "-free";
                    break;
                case Mode::end:
                    o << "-end";
                    break;

            }
            o << "  --next idx: " << table[idx].next_el << '\n';
            o << '\n';
      }
  }

template <typename Key, size_t N>
size_t ADS_set<Key,N>::erase(const key_type &key) {
    

    element *p {find_(key)};   //where key actually is
     element *pos = &table[h(key)];   //where key should be so where it first hashes
     element *pre = pos;
     element *next = p;
     std::vector<element*> els;
     size_type hash_pos;
    element *new_els;
    key_type new_key;
     if(p){   //it is in the table
      if(p == pos && pos->next_el == nullptr){  //key is at hashed pos and not part of a list
        pos->mode = Mode::free;
        --curr_size;
        return 1;
      }
      if(p!=pos){      //search for previous if not at hashed pos
        while(pre->next_el!=p){
          pre=pre->next_el;
        }
        pre->next_el=nullptr;
      }
      if(p->next_el==nullptr){ //key is at the end of a list
        p->mode=Mode::free;
        p->next_el=nullptr;
        --curr_size;
        return 1;
      }
      else{     //key is not at the end of the list
        if(next->next_el!=nullptr){    //Vector of elements
          next=next->next_el;
          while(next->next_el!=nullptr){
            els.push_back(next);
            next=next->next_el;
          }
          els.push_back(next);
        }
          p->mode=Mode::free;   //pos leeren
          p->next_el=nullptr;

          for(size_type i{0}; i<els.size(); ++i){
            new_key = els[i]->key;
            hash_pos = h(new_key);
            if(p == &table[hash_pos]){   //case 1 h meets the lucke
              p->key = new_key;
              p->mode=Mode::used;
              els[i]->mode=Mode::free;
              els[i]->next_el=nullptr;
              p = els[i];
            }
            else{     //fall 2 h meets an already abgearbeitet Element
              new_els = &table[hash_pos];
              while(new_els->next_el!=nullptr){
                new_els=new_els->next_el;
              }
              new_els->next_el= els[i];
              els[i]->next_el=nullptr;
            }
          }
      --curr_size;
      return 1;
      }
    }
    return 0;
  

}


template <typename Key, size_t N>
class ADS_set<Key,N>::Iterator {
public:
  using value_type = Key;
  using difference_type = std::ptrdiff_t;
  using reference = const value_type &;
  using pointer = const value_type *;
  using iterator_category = std::forward_iterator_tag;

private:
  element *pos;
  void skip() {while (pos->mode != Mode::used && pos->mode != Mode::end) ++pos;}
public:

  explicit Iterator(element *pos = nullptr): pos{pos} { if(pos) skip();}
  reference operator*() const { return pos->key; }
  pointer operator->() const{return &pos->key;}
  

  Iterator &operator++(){
    
    ++pos;
    
    skip();
     
     
    return *this;
    
    
  }

  Iterator operator++(int){
    auto it {*this}; 
    
    ++*this;
  
   return it;
   }

  friend bool operator==(const Iterator &lhs, const Iterator &rhs){return lhs.pos == rhs.pos;}
  friend bool operator!=(const Iterator &lhs, const Iterator &rhs){return !(lhs == rhs);}
};

template <typename Key, size_t N> void swap(ADS_set<Key,N> &lhs, ADS_set<Key,N> &rhs) { lhs.swap(rhs); }


#endif // ADS_SET_H