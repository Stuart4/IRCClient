
//
// Implementation of a HashTable that stores void *
//
#include "HashTableVoid.h"

// Obtain the hash code of a key
int HashTableVoid::hash(const char * key)
{
	long sum = 0;
	while (*key != '\0') {
		sum += (long) *key;
		key++;
	}
	return sum % TableSize;
}

// Constructor for hash table. Initializes hash table
HashTableVoid::HashTableVoid()
{
	_buckets = (HashTableVoidEntry **) malloc(sizeof(HashTableVoidEntry *) * TableSize);
	for (int i = 0; i < TableSize; i++) {
		_buckets[i] = new HashTableVoidEntry;
		_buckets[i]->_data = NULL;
		_buckets[i]->_key = NULL;
		_buckets[i]->_next = NULL;
	}

}

// Add a record to the hash table. Returns true if key already exists.
// Substitute content if key already exists.
bool HashTableVoid::insertItem( const char * key, void * data)
{
	HashTableVoidEntry * entry = _buckets[hash(key)];
	while (entry->_next != NULL && strcmp(entry->_key, key) != 0) {
		entry = entry->_next;
	}
	if (entry->_key != NULL && strcmp(entry->_key, key) == 0) {
		entry->_data = data;
		return true;
	}
	if (entry->_key != NULL && entry->_data == NULL) {
		entry->_data = data;
		entry->_key = key;
		return false;
	}
	if (entry->_data != NULL) {
		entry->_next = new HashTableVoidEntry;
		entry = entry->_next;
	}
	entry->_next = NULL;
	entry->_data = data;
	entry->_key = key;
	return false;
}

// Find a key in the dictionary and place in "data" the corresponding record
// Returns false if key is does not exist
bool HashTableVoid::find( const char * key, void ** data)
{
	HashTableVoidEntry * entry  = _buckets[hash(key)];
	while (entry->_next != NULL && entry->_key != NULL && strcmp(entry->_key, key) != 0) {
		entry = entry->_next;
	}
	if (entry->_key == NULL) {
		return false;
	}
	if (entry->_key != NULL && strcmp(entry->_key, key) == 0) {
		*data = entry->_data;
		return true;
	}
	return false;
}

// Removes an element in the hash table. Return false if key does not exist.
bool HashTableVoid::removeElement(const char * key)
{
	HashTableVoidEntry * entry  = _buckets[hash(key)];
	while (entry->_next != NULL && entry->_key != NULL && strcmp(entry->_key, key) != 0) {
		entry = entry->_next;
	}
	if (entry->_key == NULL) return false;
	if (strcmp(entry->_key, key) == 0) {
		entry->_key = NULL;
		entry->_data = NULL;
		entry->_next = NULL;
		return true;
	}
	return false;
}

// Creates an iterator object for this hash table
HashTableVoidIterator::HashTableVoidIterator(HashTableVoid * hashTable)
{
	_hashTable = hashTable;
	_currentBucket = 0;
	_currentEntry = _hashTable->_buckets[0];

}

// Returns true if there is a next element. Stores data value in data.
bool HashTableVoidIterator::next(const char * & key, void * & data)
{
	while (_currentBucket != 2039 && _currentEntry->_data == NULL) {
		if (_currentEntry->_next != NULL) {
			_currentEntry = _currentEntry->_next;
		} else {
			if (_currentBucket == 2039) return false;
			_currentEntry = _hashTable->_buckets[++_currentBucket];
		}
	}
	if (_currentBucket == 2039) return false;
	data = _currentEntry->_data;
	key = _currentEntry->_key;
	_currentEntry = _hashTable->_buckets[++_currentBucket];
	return true;
}

