#ifndef DLINKEDLIST_H_
#define DLINKEDLIST_H_

#include <iostream>

template <typename T>
struct Node {
  T data;
  Node<T> *next;
  Node<T> *prev;

  void print() {
    std::cout << data;
  }
};

template <typename T>
class DLinkedList {
  private:
  Node<T> *head;
  Node<T> *tail;
  public:
  DLinkedList() {
    head = nullptr;
    tail = nullptr;
  };
  ~DLinkedList() {
      while (!isEmpty()) {
          RemoveHead();
      }
  }
  void RemoveHead() {
    if (head == nullptr) return;
    Node<T>* temp = head;
    if (head->next == nullptr)
      head = nullptr;
    else {
      head=head->next;
      head->prev = nullptr;
    }
    delete temp;
  };
  void RemoveTail() {
    if (tail == nullptr) return;
    else if (size() == 1) {
      RemoveHead();
      return;
    }
    Node<T>* temp = tail;
    if (tail->prev == nullptr)
      tail = nullptr;
    else {
      tail=tail->prev;
      tail->next = nullptr;
    }
    delete temp;
  };
  void AddHead(T data) {
    Node<T> *val = new Node<T>;
    val->data = data;
    val->prev = nullptr;
    if (isEmpty()) {
      tail = val;
      head = val;
      val->next = nullptr;
    } else {
      val->next = head;
      head->prev = val;
    }
    head = val;
  };
  bool isEmpty() {
    return head == nullptr;
  }
  void AddTail(T data) {
    if (isEmpty()) {
      AddHead(data);
      return;
    } else {
      Node<T>* val = new Node<T>;
      val->data = data;
      val->next = nullptr;
      val->prev = tail;
      tail->next = val;
      tail = val;
    }
  };
  T& operator[](int index) {
    if (index >= size()) {
        throw std::out_of_range("Index out of bounds");
    }
    Node<T> *temp = head;
    int i = 0;
    while (i < index) {
      temp=temp->next;
      i++;
    }
    return temp->data;
  };
  std::size_t size(Node<T> *temp) {
    if (temp == nullptr) return 0;
    return size(temp->next) + 1;
  };
  std::size_t size() {
    return size(head);
  };

  Node<T>* getHead() {
    return head;
  }
  Node<T>* getTail() {
    return tail;
  }
};

#endif
