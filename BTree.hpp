#include "utility.hpp"
#include <functional>
#include <cstddef>
#include "exception.hpp"
#include <map>
#include <stdio.h>
namespace sjtu
{
	template <class Key, class Value, class Compare = std::less<Key> >
	class BTree
	{
		//private:
		//    // Your private members go here
	public:
		typedef pair<Key, Value> value_type;

		static const int M = 1000;                // need modify
		static const int L = 200;                 // need modify
		static const int MMIN = M / 2;            // M / 2
		static const int LMIN = L / 2;            // L / 2

		struct basicinfo
		{
			int head;          // head of leaf
			int tail;          // tail of leaf
			int root;          // root of Btree
			int size;          // size of Btree
			int eof;         // end of file
			basicinfo() {
				head = 0;
				tail = 0;
				root = 0;
				size = 0;
				eof = 0;
			};
		};
		class iterator;
		struct leafnode {
			int offset;          // offset
			int parent;               // parent
			int prev, next;          // previous and next leaf
			int num;                  // number of pairs in leaf
			value_type data[L + 1];   // data
			leafnode() {
				offset = 0, parent = 0, prev = 0, next = 0, num = 0;
			}
		};
		struct internalnode 
		{
			int offset;      	// offset
			int parent;           	// parent
			int ch[M + 1];     	// children
			Key key[M + 1];   	// key
			int num;              	//child number in internal node
			bool type;            	// child is leaf or not
			internalnode() 
			{
				offset = 0, parent = 0;
				for (int i = 0; i <= M; ++i) ch[i] = 0;
				num = 0;
				type = 0;
			}
		};

		FILE *file;
		bool ifopen;
		//char filename[10]="btreedata";
		bool ifexist;
		basicinfo info;

		void read(void *place, int offset, int num, int size)
		{
			if (fseek(file, offset, 0))
				throw 1;
			fread(place, size, num, file);
		}
		void write(void *place, int offset, int num, int size)
		{
			if (fseek(file, offset, 0))
				throw 1;
			fwrite(place, size, num, file);
		}
		void openfile()
		{
			ifexist = 1;
			if (ifopen == 0)
			{
				file = fopen("btreedata", "rb+");
				if (file == nullptr)
				{
					ifexist = 0;
					file = fopen("btreedata", "w");
					fclose(file);
					file = fopen("btreedata", "rb+");
				}
				else
				{
					read(&info, 0, 1, sizeof(basicinfo));
				}
				ifopen = 1;
			}
		}
		void closefile()
		{
			if (ifopen == 1)
			{
				fclose(file);
				ifopen = 0;
			}
		}
		void build()
		{
			info.size = 0;
			info.root = info.eof = sizeof(basicinfo);
			internalnode root;
			leafnode leaf;
			root.offset = info.eof;
			info.eof += sizeof(internalnode);
			info.head = info.tail = leaf.offset = info.eof;
			info.eof += sizeof(leafnode);
			root.type = 1;
			root.num = 1;
			root.ch[0] = leaf.offset;
			leaf.parent = root.offset;
			write(&info, 0, 1, sizeof(basicinfo));
			write(&root, root.offset, 1, sizeof(internalnode));
			write(&leaf, leaf.offset, 1, sizeof(leafnode));
		}
		int findleaf(const Key &key, int offset)
		{
			internalnode p;
			read(&p, offset, 1, sizeof(internalnode));
			if (p.type == 1)
			{
				int i = 0;
				for (; i < p.num; ++i)
				{
					if (key < p.key[i])
						break;
				}
				if (i == 0)
					return 0;
				else
					return p.ch[i - 1];
			}
			else
			{
				int i = 0;
				for (; i < p.num; ++i)
				{
					if (key < p.key[i])
						break;
				}
				if (i == 0)
					return 0;
				else
					return findleaf(key, p.ch[i - 1]);
			}
		}
		//void insertnode(internalnode &node, Key &key, int ch);
		void splitleaf(leafnode &leaf)
		{
			leafnode newleaf;
			newleaf.num = leaf.num - leaf.num / 2;
			leaf.num /= 2;
			newleaf.offset = info.eof;
			info.eof += sizeof(leafnode);
			newleaf.parent = leaf.parent;
			for (int i = 0; i < newleaf.num; ++i)
			{
				newleaf.data[i].first = leaf.data[i + leaf.num].first;
				newleaf.data[i].second = leaf.data[i + leaf.num].second;
			}
			newleaf.next = leaf.next;
			newleaf.prev = leaf.offset;
			leaf.next = newleaf.offset;
			if (newleaf.next == 0)
				info.tail = newleaf.offset;
			else
			{
				leafnode tmp;
				read(&tmp, newleaf.next, 1, sizeof(leafnode));
				tmp.prev = newleaf.offset;
				write(&tmp, tmp.offset, 1, sizeof(leafnode));
			}
			write(&info, 0, 1, sizeof(basicinfo));
			write(&leaf, leaf.offset, 1, sizeof(leafnode));
			write(&newleaf, newleaf.offset, 1, sizeof(leafnode));

			internalnode p;
			read(&p, leaf.parent, 1, sizeof(internalnode));
			insertnode(p,newleaf.data[0].first,newleaf.offset);
		}


		pair <iterator, OperationResult> insertleaf(leafnode &leaf,const Key &key,const Value &value)
		{
			int i = 0;
			for (; i < leaf.num; ++i)
			{
				if (key == leaf.data[i].first)
					return pair <iterator, OperationResult>(iterator(),Fail);
				if (key < leaf.data[i].first)
					break;
			}
			for (int j = leaf.num - 1; j >= i; --j)
			{
				leaf.data[j + 1].first = leaf.data[j].first;
				leaf.data[j + 1].second = leaf.data[j].second;
			}
			leaf.data[i].first = key;
			leaf.data[i].second = value;
			++leaf.num;
			++info.size;
			write(&info, 0, 1, sizeof(basicinfo));
			if (leaf.num <= L)
				write(&leaf, leaf.offset, 1, sizeof(leafnode));
			else
				splitleaf(leaf);
			return  pair <iterator, OperationResult>(iterator(), Success);
		}

		void splitnode(internalnode &node)
		{
			internalnode newnode;
			newnode.num = node.num - node.num / 2;
			node.num /= 2;
			newnode.parent = node. parent;
			newnode.type = node.type;
			newnode.offset = info.eof;
			info.eof += sizeof(internalnode);
			for (int i = 0; i < newnode.num; ++i)
				newnode.key[i] = node.key[i + node.num];
			for (int i = 0; i < newnode.num; ++i)
				newnode.ch[i] = node.ch[i + node.num];
			leafnode tleaf;
			internalnode tnode;
			if (newnode.type == 1)
			{
				for (int i = 0; i < newnode.num; ++i)
				{
					read(&tleaf, newnode.ch[i], 1, sizeof(leafnode));
					tleaf.parent = newnode.parent;
					write(&tleaf, tleaf.offset, 1, sizeof(leafnode));
				}
			}
			else
			{
				for (int i = 0; i < newnode.num; ++i)
				{
					read(&tnode, newnode.ch[i], 1, sizeof(internalnode));
					tnode.parent = newnode.parent;
					write(&tnode, tnode.offset, 1, sizeof(internalnode));
				}
			}
			if (info.root != node.offset)
			{
				write(&info, 0, 1, sizeof(basicinfo));
				write(&node, node.offset, 1, sizeof(internalnode));
				write(&newnode, newnode.offset, 1, sizeof(internalnode));

				internalnode p;
				read(&p, node.parent, 1, sizeof(internalnode));
				insertnode(p, newnode.key[0], newnode.offset);
			}
			else
			{
				internalnode newrt;
				newrt.parent = 0;
				newrt.type = 0;
				newrt.offset = info.eof;
				info.eof += sizeof(internalnode);
				newrt.num = 2;
				newrt.key[0] = node.key[0];
				newrt.ch[0] = node.offset;
				newrt.key[1] = newnode.key[0];
				newrt.ch[1] = newnode.offset;
				node.parent = newrt.offset;
				newnode.parent = newrt.offset;
				info.root = newrt.offset;
				write(&info, 0, 1, sizeof(basicinfo));
				write(&node, node.offset, 1, sizeof(internalnode));
				write(&newnode, newnode.offset, 1, sizeof(internalnode));
				write(&newrt, newrt.offset, 1, sizeof(internalnode));
			}
		}
		void insertnode(internalnode &node, Key &key, int ch)
		{
			int i = 0;
			for (; i < node.num; ++i)
			{
				if (key < node.key[i])break;
			}
			for (int j = node.num - 1; j >= i; --j)
			{
				node.key[i + 1] = node.key[j];
			}
			for (int j = node.num - 1; j >= i; --j)
			{
				node.ch[i + 1] = node.ch[j];
			}
			node.key[i] = key;
			node.ch[i] = ch;
			if (node.num <= M)
			{
				write(&node, node.offset, 1, sizeof(internalnode));
			}
			else
				splitnode(node);
		}

		bool borrowrleaf(leafnode leaf)
		{
			if (leaf.next = 0)
				return 0;
			leafnode rb;
			read(&rb, leaf.next, 1, sizeof(leafnode));
			if (leaf.parent != rb.parent || rb.num <= LMIN)
				return 0;
			Key k1, k2;
			k1 = rb.data[0].first;
			k2 = rb.data[1].first;
			leaf.data[leaf.num].first = rb.data[0].first;
			leaf.data[leaf.num++].second = rb.data[0].second;
			--rb.num;
			for (int i = 0; i < rb.num; ++i)
			{
				rb.data[i].first = rb.data[i + 1].first;
				rb.data[i].second = rb.data[i + 1].second;
			}

			internalnode p;
			read(&p, leaf.parent, 1, sizeof(internalnode));
			for (int i = 0; i < p.num; ++i) {
				if (p.key[i] == k1)
				{
					p.key[i] = k2;
					break;
				}
			}
			write(&p, p.offset, 1, sizeof(internalnode));
			write(&leaf, leaf.offset, 1, sizeof(leafnode));
			write(&rb, rb.offset, 1, sizeof(leafnode));
			return 1;
		}
		bool borrowlleaf(leafnode leaf)
		{
			if (leaf.prev = 0)
				return 0;
			leafnode lb;
			read(&lb, leaf.prev, 1, sizeof(leafnode));
			if (leaf.parent != lb.parent || lb.num <= LMIN)
				return 0;
			Key k1, k2;
			k1 = leaf.data[0].first;
			k2 = lb.data[lb.num - 1].first;
			for (int i = leaf.num - 1; i >= 0; --i)
			{
				leaf.data[i + 1].first = leaf. data[i].first;
				leaf.data[i + 1].second = leaf.data[i].second;
			}
			leaf.data[0].first = lb.data[lb.num - 1].first;
			leaf.data[0].second = lb.data[lb.num - 1].second;
			--lb.num;
			++leaf.num;

			internalnode p;
			read(&p, leaf.parent, 1, sizeof(internalnode));
			for (int i = 0; i < p.num; ++i) {
				if (p.key[i] == k1)
				{
					p.key[i] = k2;
					break;
				}
			}
			write(&p, p.offset, 1, sizeof(internalnode));
			write(&leaf, leaf.offset, 1, sizeof(leafnode));
			write(&lb, lb.offset, 1, sizeof(leafnode));
			return 1;
		}
		bool checknode(internalnode node)
		{
			if (node.parent == 0) return 1;
			if (node.num >= MMIN) return 1;
			return 0;
		}
		//void operatenode(internalnode node);
		bool borrowrnode(internalnode node)
		{
			if (node.parent == 0)return 0;
			internalnode p;
			read(&p, node.parent, 1, sizeof(internalnode));
			int i = 0;
			for (; i < p.num; ++i)
			{
				if (p.ch[i] == node.offset)break;
			}
			if (i >= p.num - 1)return 0;
			internalnode rb;
			read(&rb, p.ch[i + 1], 1, sizeof(internalnode));
			if (rb.num <= MMIN) return 0;

			node.key[node.num] = rb.key[0];
			node.ch[node.num] = rb.ch[0];
			++node.num;
			for (int j = 0; j < rb.num - 1; ++j)
			{
				rb.key[j] = rb.key[j + 1];
				rb.ch[j] = rb.ch[j + 1];
			}
			--rb.num;
			p.key[i + 1] = rb.key[0];
			if (node.type == 1)
			{
				leafnode leaf;
				read(&leaf, node.ch[node.num - 1], 1, sizeof(leafnode));
				leaf.parent = node.offset;
				write(&leaf, leaf.offset, 1, sizeof(leafnode));
			}
			else
			{
				internalnode son;
				read(&son, node.ch[node.num - 1], 1, sizeof(internalnode));
				son.parent = node.offset;
				write(&son, son.offset, 1, sizeof(internalnode));
			}
			write(&node, node.offset, 1, sizeof(internalnode));
			write(&rb, rb.offset, 1, sizeof(internalnode));
			write(&p, p.offset, 1, sizeof(internalnode));
			return 1;
		}
		bool borrowlnode(internalnode node)
		{
			if (node.parent == 0)return 0;
			internalnode p;
			read(&p, node.parent, 1, sizeof(internalnode));
			int i = 0;
			for (; i < p.num; ++i)
			{
				if (p.ch[i] == node.offset)break;
			}
			if (i == 0)return 0;
			internalnode lb;
			read(&lb, p.ch[i - 1], 1, sizeof(internalnode));
			if (lb.num <= MMIN) return 0;
			for (int j = node.num - 1; j >= 0; --j)
			{
				node.key[j + 1] = node.key[j];
				node.ch[j + 1] = node.ch[j];
			}
			node.key[0] = lb.key[lb.num - 1];
			node.ch[0] = lb.ch[lb.num - 1];
			++node.num;
			--lb.num;

			p.key[i] = node.key[0];
			if (node.type == 1)
			{
				leafnode leaf;
				read(&leaf, node.ch[0], 1, sizeof(leafnode));
				leaf.parent = node.offset;
				write(&leaf, leaf.offset, 1, sizeof(leafnode));
			}
			else
			{
				internalnode son;
				read(&son, node.ch[0], 1, sizeof(internalnode));
				son.parent = node.offset;
				write(&son, son.offset, 1, sizeof(internalnode));
			}
			write(&node, node.offset, 1, sizeof(internalnode));
			write(&lb, lb.offset, 1, sizeof(internalnode));
			write(&p, p.offset, 1, sizeof(internalnode));
			return 1;
		}
		bool mergernode(internalnode node)
		{
			if (node.parent == 0)return 0;
			internalnode p;
			read(&p, node.parent, 1, sizeof(internalnode));
			int i = 0;
			for (; i < p.num; ++i)
			{
				if (p.ch[i] == node.offset)break;
			}
			if (i >= p.num - 1)return 0;
			internalnode rb;
			read(&rb, p.ch[i + 1], 1, sizeof(internalnode));
			for (int j = 0; j < rb.num; ++j)
			{
				node.key[node.num] = rb.key[j];
				node.ch[node.num] = rb.ch[j];
				if (node.type == 1) {
					leafnode son;
					read(&son, node.ch[node.num], 1, sizeof(leafnode));
					son.parent = node.offset;
					write(&son, son.offset, 1, sizeof(leafnode));
				}
				else
				{
					internalnode son;
					read(&son, node.ch[node.num], 1, sizeof(internalnode));
					son.parent = node.offset;
					write(&son, son.offset, 1, sizeof(internalnode));
				}
				++node.num;
			}
			for (int j = i + 1; j < p.num - 1; ++j)
			{
				p.key[j] = p.key[j + 1];
				p.ch[j] = p.ch[j + 1];
			}
			--p.num;
			write(&node, node.offset, 1, sizeof(internalnode));
			if (checknode(p) == 1)
			{
				write(&p, p.offset, 1, sizeof(internalnode));
			}
			else
				operatornode(p);
			return 1;
		}
		bool mergelnode(internalnode node)
		{
			if (node.parent == 0)return 0;
			internalnode p;
			read(&p, node.parent, 1, sizeof(internalnode));
			int i = 0;
			for (; i < p.num; ++i)
			{
				if (p.ch[i] == node.offset)break;
			}
			if (i == 0)return 0;
			internalnode lb;
			read(&lb, p.ch[i - 1], 1, sizeof(internalnode));
			for (int j = 0; j < node.num; ++j)
			{
				lb.key[lb.num] = node.key[j];
				lb.ch[lb.num] = node.ch[j];
				if (lb.type == 1)
				{
					leafnode son;
					read(&son, lb.ch[lb.num], 1, sizeof(leafnode));
					son.parent = lb.offset;
					write(&son, son.offset, 1, sizeof(leafnode));
				}
				else
				{
					internalnode son;
					read(&son, lb.ch[lb.num], 1, sizeof(internalnode));
					son.parent = lb.offset;
					write(&son, son.offset, 1, sizeof(internalnode));
				}
				++lb.num;
			}
			for (int j = i; j < p.num - 1; ++j)
			{
				p.key[j] = p.key[j + 1];
				p.ch[j] = p.ch[j + 1];
			}
			--p.num;
			write(&lb, lb.offset, 1, sizeof(internalnode));
			if (checknode(p) == 1)
			{
				write(&p, p.offset, 1, sizeof(internalnode));
			}
			else
				operatornode(p);
			return 1;
		}
		void operatenode(internalnode node)
		{
			if (borrowrnode(node) == 1) return;
			if (borrowlnode(node) == 1) return;
			if (mergernode(node) == 1) return;
			if (mergelnode(node) == 1) return;
			internalnode p;
			read(&p, node.parent, 1, sizeof(internalnode));
			if (p.parent == 0) {
				info.root = node.offset;
				node.parent = 0;
				write(&info, 0, 1, sizeof(basicinfo));
				write(&node, node.offset, 1, sizeof(internalnode));
			}
			else
			{
				internalnode pp;
				read(&pp, p.parent, 1, sizeof(internalnode));
				for (int i = 0; i < pp.num; ++i)
					if (pp.ch[i] == p.offset)
					{
						pp.ch[i] = node.offset;
						break;
					}
				node.parent = pp.offset;
				write(&pp, pp.offset, 1, sizeof(internalnode));
				write(&node, node.offset, 1, sizeof(internalnode));
			}
		}

		bool mergerleaf(leafnode leaf)
		{
			if (leaf.next = 0)
				return 0;
			leafnode rb;
			read(&rb, leaf.next, 1, sizeof(leafnode));
			if (leaf.parent != rb.parent)
				return 0;
			for (int i = 0; i < rb.num; ++i)
			{
				leaf.data[leaf.num + i].second = rb.data[i].first;
				leaf.data[leaf.num + i].second = rb.data[i].second;
			}
			leaf.num += rb.num;
			leaf.next = rb.next;
			if (rb.offset == info.tail)
			{
				info.tail = leaf.offset;
				write(&info, 0, 1, sizeof(info));
			}
			else
			{
				leafnode tmp;
				read(&tmp.leaf.next, 1, sizeof(leafnode));
				tmp.prev = leaf.offset;
				write(&tmp, tmp.offset, 1, sizeof(leafnode));
			}
			internalnode p;
			read(&p, leaf.parent, 1, sizeof(internalnode));
			int i = 0;
			for (; i < p.num; ++i)
			{
				if (p.key[i] == rb.data[0].first)
					break;
			}
			for (int j = i; j < p.num - 1; ++j)
			{
				p.key[i] == p.key[i + 1];
				p.ch[i] == p.ch[i + 1];
			}
			--p.num;
			write(&leaf, leaf.offset, 1, sizeof(leafnode));
			if (checknode(p) == 1) write(&p, p.offset, 1, sizeof(internalnode));
			else operatenode(p);
			return 1;
		}
		bool mergelleaf(leafnode leaf)
		{
			if (leaf.prev = 0)
				return 0;
			leafnode lb;
			read(&lb, leaf.prev, 1, sizeof(leafnode));
			if (leaf.parent != lb.parent)
				return 0;
			for (int i = 0; i < leaf.num; ++i)
			{
				lb.data[leaf.num + i].second = leaf.data[i].first;
				lb.data[leaf.num + i].second = leaf.data[i].second;
			}
			lb.num += leaf.num;
			lb.next = leaf.next;
			if (leaf.offset == info.tail)
			{
				info.tail = lb.offset;
				write(&info, 0, 1, sizeof(info));
			}
			else
			{
				leafnode tmp;
				read(&tmp.lb.next, 1, sizeof(leafnode));
				tmp.prev = lb.offset;
				write(&tmp, tmp.offset, 1, sizeof(leafnode));
			}
			internalnode p;
			read(&p, leaf.parent, 1, sizeof(internalnode));
			int i = 0;
			for (; i < p.num; ++i)
			{
				if (p.key[i] == leaf.data[0].first)
					break;
			}
			for (int j = i; j < p.num - 1; ++j)
			{
				p.key[i] == p.key[i + 1];
				p.ch[i] == p.ch[i + 1];
			}
			--p.num;
			write(&lb, lb.offset, 1, sizeof(leafnode));
			if (checknode(p) == 1) write(&p, p.offset, 1, sizeof(internalnode));
			else operatenode(p);
			return 1;
		}
		void operateleaf(leafnode leaf)
		{
			if (borrowrleaf(leaf) == 1) return;
			if (borrowlleaf(leaf) == 1) return;
			if (mergerleaf(leaf) == 1) return;
			if (mergelleaf(leaf) == 1) return;
			write(&leaf, leaf.offset, 1, sizeof(leafnode));
		}
		class const_iterator;
		class iterator {
			//private:
			//    // Your private members go here
		public:
			/*BTree* tree;
			int offset;
			int index;*/
			bool modify(const Value& value) {
				return 0;
			}
			iterator() {

			};
			//iterator(iterator &other) {};
			/*iterator(const iterator& other) {
				
			}*/
			// Return a new iterator which points to the n-next elements
			iterator operator++(int) {
				// Todo iterator++
				/*return nullptr;*/
			}
			iterator& operator++() {
				// Todo ++iterator
				/*return nullptr;*/
			}
			iterator operator--(int) {
				// Todo iterator--
				/*return nullptr;*/
			}
			iterator& operator--() {
				// Todo --iterator
				//return nullptr;
			}
			// Overloaded of operator '==' and '!='
			// Check whether the iterators are same
			bool operator==(const iterator& rhs) const {
				// Todo operator ==
				return 0;
			}
			bool operator==(const const_iterator& rhs) const {
				// Todo operator ==
				return 0;
			}
			bool operator!=(const iterator& rhs) const {
				// Todo operator !=
				return 0;
			}
			bool operator!=(const const_iterator& rhs) const {
				// Todo operator !=
				return 0;
			}
		};
		class const_iterator {
			// it should has similar member method as iterator.
			//  and it should be able to construct from an iterator.
		private:
			// Your private members go here
		public:
			const_iterator() {
				// TODO

			}
			const_iterator(const const_iterator& other) {

			}
			const_iterator(const iterator& other) {
				 
			}
			// And other methods in iterator, please fill by yourself.
		};
		// Default Constructor and Copy Constructor
		BTree() {
			// Todo Default
			file= nullptr;
			openfile();
			if (ifexist == 1)
			{
				build();
			}
		}
		BTree(const BTree& other) {
			openfile();
			read(&info, 0, 1, sizeof(basicinfo));
		}
		BTree& operator=(const BTree& other) {
			openfile();
			read(&info, 0, 1, sizeof(basicinfo));
		}
		~BTree() {
			closefile();
		}
		// Insert: Insert certain Key-Value into the database
		// Return a pair, the first of the pair is the iterator point to the new
		// element, the second of the pair is Success if it is successfully inserted

		pair<iterator, OperationResult> insert(const Key& key, const Value& value) {
			int index = findleaf(key, info.root);
			leafnode leaf;
			if (info.size == 0 || index == 0)
			{
				read(&leaf, info.head, 1, sizeof(leafnode));
				pair <iterator, OperationResult> ret = insertleaf(leaf,key, value);
				if (ret.second == Fail)return ret;
				int x = leaf.parent;
				internalnode node;
				while (x != 0)
				{
					read(&node, x, 1, sizeof(internalnode));
					node.key[0] = key;
					write(&node, x, 1, sizeof(internalnode));
					x = node.parent;
				}
				return ret;
			}
			read(&leaf, info.head, 1, sizeof(leafnode));
			pair <iterator, OperationResult> ret = insertleaf(leaf,key, value);
			return ret;

		}
		// Erase: Erase the Key-Value
		// Return Success if it is successfully erased
		// Return Fail if the key doesn't exist in the database
		OperationResult erase(const Key& key)
		{
			// TODO erase function
			return Fail;  // If you can't finish erase part, just remaining here.
		}
		// Return a iterator to the beginning
		iterator begin() {
			//return nullptr;
		}
		const_iterator cbegin() const
		{ 
			//return nullptr;
		}
		// Return a iterator to the end(the next element after the last)
		iterator end() { //return nullptr; 
		}
		const_iterator cend() const {
			//return nullptr;
		}
		// Check whether this BTree is empty
		bool empty() const { return info.size == 0; }
		// Return the number of <K,V> pairs
		size_t size() const { return (size_t)info.size; }
		// Clear the BTree
		void clear() 
		{
			file = fopen("btreedata", "w");
			fclose(file);
			openfile();
			build();
		}
		// Return the value refer to the Key(key)
		Value at(const Key& key) {
			int index = findleaf(key, info.root);
			leafnode leaf;
			read(&leaf, index, 1, sizeof(leafnode));
			for (int i = 0; i < leaf.num; ++i)
			{
				if (leaf.data[i].first == key)
				{
					return  leaf.data[i].second;
				}
			}
			return 0;
		}
		/**
		 * Returns the number of elements with key
		 *   that compares equivalent to the specified argument,
		 * The default method of check the equivalence is !(a < b || b > a)
		 */
		size_t count(const Key& key) const {
			return 0;
		}
		/**
		 * Finds an element with key equivalent to key.
		 * key value of the element to search for.
		 * Iterator to an element with key equivalent to key.
		 *   If no such element is found, past-the-end (see end()) iterator is
		 * returned.
		 */
		iterator find(const Key& key) {
			//return nullptr;
		}
		const_iterator find(Key& key) const {
			//return nullptr;
		}
	};
}
// namespace sjtu
