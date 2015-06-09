#include "IRCVector.h"
#include <string.h>
#include <string>

using namespace std;

IRCVector::IRCVector() {
	size = 0;
	head = NULL;
	limited = false;

}
bool IRCVector::find(string toFind) {
	if (head == NULL) return false;
	Node * cursor = head;
	while (cursor != NULL) {
		if (cursor->data->compare(toFind) == 0) return true;
		cursor = cursor->right;
	}
	return false;
}
void IRCVector::insert(string * toInsert) {
	string * newData = new string();
	newData->append(*toInsert);
	if (head == NULL) {
		head = new Node();
		head->left = NULL;
		head->right = NULL;
		head->data = newData;
		return;
	}
	Node * oldHead = head;
	head = new Node();
	head->left = NULL;
	head->right = oldHead;
	head->data = newData;
	oldHead->left = head;
	size++;
	if (limited && getSize() == 101) {
		Node * toDie = head;
		while (toDie->right != NULL) toDie = toDie->right;
		toDie->left->right = NULL;
		delete toDie;
	}
}
string IRCVector::sort() {
	Node * cursor = head;
	string out;
	string elems[getSize()];
	for (int i = 0; i < getSize(); i++) {
		elems[i] = *cursor->data;
		cursor = cursor->right;
	}
	for (int i = 0; i < getSize() - 1; i++) {
		if (elems[i].compare(elems[i+1]) > 0) {
			string tmp = elems[i];
			elems[i] = elems[i+1];
			elems[i+1] = tmp;
			i = -1;
		}
	}
	for (int i = 0; i < getSize(); i++) {
		out.append(elems[i]).append("\r\n");
	}
	return out;
}
void IRCVector::remove(string toRemove) {
	Node * cursor = head;
	while (cursor != NULL) {
		if (cursor->data->compare(toRemove) == 0) {
			if (cursor == head) {
				if (head->right == NULL) {
					delete head;
					head = NULL;
				} else {
					delete head;
					head = head->right;
				}
				return;
			}
			if (cursor->left != NULL)
				cursor->left->right = cursor->right;
			if (cursor->right != NULL)
				cursor->right->left = cursor->left;
			delete cursor;
			size--;
			return;
		}
		cursor = cursor->right;
	}
}
std::string IRCVector::toString() {
	string out;
	Node * cursor = head;
	while (cursor != NULL) {
		out.append(*cursor->data).append("\r\n");
		cursor = cursor->right;
	}
	return out;
}
std::string IRCVector::at(int i) {
	return "why do youuu call?";
}
int IRCVector::getSize() {
	int out = 0;
	Node * cursor = head;
	while (cursor != NULL) {
		cursor = cursor->right;
		out++;
	}
	return out;
}
IRCVectorIterator::IRCVectorIterator(IRCVector * vector) {
	cursor = vector->head;
	while (cursor != NULL && cursor->right != NULL) cursor = cursor->right;
	pos = 0;
}
bool IRCVectorIterator::next(string * & data) {
	if (cursor == NULL) {
		data = NULL;
		return false;
	}
	data = cursor->data;
	cursor = cursor->right;
	return true;
}
