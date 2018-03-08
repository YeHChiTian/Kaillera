/******************************************************************************
***N***N**Y***Y**X***X*********************************************************
***NN**N***Y*Y****X*X**********************************************************
***N*N*N****Y******X***********************************************************
***N**NN****Y*****X*X************************ Make Date  : Jun 29, 2007 *******
***N***N****Y****X***X*********************** Last Update: Jun 29, 2007 *******
******************************************************************************/
#pragma once

#include <string>
#pragma intrinsic (memcpy)

template <class _Type, int _Size>
class oslist {
public:
	_Type items[_Size];	
	int length;	

public:

	oslist(){
		length = 0;
	}

	//add 	adds an element to the list
	void add(_Type element){
		items[length++] = element;
	}

	//remove 	removes an element from the list
	void remove(int i){
		if (i >= 0 && i < length) {
			int l = length-1;
			if (l!=i) {
				memcpy(&items[i], &items[i+1], (l-i) * sizeof(_Type));
			}
			length = l;
		}
	}

	void remove(_Type t){
		int m = length;
		for (int i = 0; i < m; i++) {
			if (items[i] == t) {
				remove(i);
				i--;
			}
		}
	}

	//set 	sets an element value in the list
	void set(_Type v, int i){
		items[i] = v;
	}

	//get 	gets an element value in the list
	_Type get(int i){
		return items[i];
	}

	//clear 	removes all elements from the list	
	void clear(){
		length = 0;
	}

	//size 	returns the number of items in the list
	int size(){
		return length;
	}
};
