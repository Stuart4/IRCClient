#ifndef IRCVECTOR_H
#define IRCVECTOR_H
#include <string>
class IRCVector {
	friend class IRCVectorIterator;
	class Node {
		public:
			std::string * data;
			Node * left;
			Node * right;
	};
	private:
		int size;
		Node * head;

	public:
		IRCVector();
		bool find(std::string toFind);
		void insert(std::string * toInsert);
		void remove(std::string toRemove);
		std::string sort();
		std::string toString();
		std::string at(int i);
		int getSize();
		bool limited;

};
class IRCVectorIterator {
	int pos;
	public:
	IRCVectorIterator(IRCVector * vector);
	bool next(std::string * & data);
	private:
	IRCVector::Node * cursor;
};
#endif
