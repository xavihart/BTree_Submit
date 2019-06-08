#include "utility.hpp"

#include <functional>
#include<fstream>
#include<iostream>

#include <cstddef>
#include<cmath>
#include "exception.hpp"

#include <map>
using namespace std;

namespace sjtu {

    template <class Key, class Value, class Compare = std::less<Key> >

    class BTree {
        public:
         class iterator;
         class const_iterator;

    private:
   

     static const int M = 4096/(sizeof(Key)+sizeof(Value))+1;
     static  const int L = M;
 
     static  const int MAX_BLOCK_SIZE=M;
     static const int MIN_BLOCK_SIZE=M/2;
     std::fstream op;
     int info_offset = 0;
     char file_name[1000];
         typedef pair <Key, Value> value_type;

       struct basicInfo{
         
          int size;

          int root;

          int head;
          
          int tail;

          int eof ;

     

          basicInfo(){
              size=0;
              root=0;
              head=0;
              tail=0;
              eof=0;
            
          }
       };
       

       struct leafnode{
           int offset;
           int prev,next;
           int cnt;
           int parent ;
           value_type data[L+1];
                 leafnode(){
               offset=0;
               prev=0;
               next=0;
           
               cnt=0;
           }
           
       };

       struct odnode{
           int offset;
           int parent;
           int child[M+5];
           int key[M+5];
           bool node_type;   ///  to check if the child of this node are leafs;
           int cnt;
           odnode(){
            offset = 0;
            parent = 0;
            node_type = 0;
            cnt = 0;
            for(int i = 0; i<M+1 ;++i) {child[i]=0;}
           }

       };

       /*********************
        *  tips:
        *  key[0],key[1],key[2].....key[size-1]
        *  then data is the position of the block in the file 
        *  whose key_value is smaller than that of key[i];
        *  however if (i=size-1) , then data[i] refer to the element 
        *  'S key value is bigger than key[i]; 
        *
       *********************/
      
      
   
 
      inline void write_leafnode(leafnode&p,int offset){
          if (op.fail())  op.close();
          if (!op.is_open())   op.open(file_name,std::ios::in|std::ios::out|std::ios::binary|std::ios::trunc);
          op.seekp(offset);
          op.write(reinterpret_cast<char *> (&p),sizeof(leafnode));
          op.flush();
      }

      inline void write_odnode(odnode&p,int offset){
          if (op.fail())  op.close();
          if (!op.is_open()) op.open(file_name,std::ios::in|std::ios::out|std::ios::binary|std::ios::trunc);
          op.seekp(offset);
          op.write(reinterpret_cast<char *>(&p),sizeof(odnode));
          op.flush();
      }

      inline void write_basic_information(basicInfo& b,int offset){

           if(op.fail()) op.close();
           if(!op.is_open()) op.open(file_name,std::ios::in|std::ios::out|std::ios::binary|std::ios::trunc);

           op.seekp(offset);
           op.write(reinterpret_cast<char*>(&b) ,sizeof(basicInfo));
           op.flush();
      }

      

    




  /* given a key find the leaf,we 
     use recursion to realize that  */

      
      int find_leaf(int offset ,Key k){

         // cout<<"call function find !<<"<<endl;
             odnode tmp;
             op.seekg(offset);
             op.read(reinterpret_cast<char *> (&tmp),sizeof(odnode));
            if(tmp.node_type == 1){ 

                   int pos = 0;
                   for( pos=0;pos<tmp.cnt;++pos)
                     if( k< tmp.key[pos] ) break;

                     if(pos==0) return 0;
                     else return tmp.child[pos-1] ;  
                    

            }else {
                int pos=0;
                for(pos=0;pos<tmp.cnt;++pos)
                  if(k<tmp.key[pos]) break;
                  if(pos==0)  return 0;
                  return find_leaf(tmp.child[pos-1],k);

            }

          

      }
/*  this function is used to insert a <key,value> pair into a certain leaf
*/
      pair<iterator ,OperationResult> insert_leaf(leafnode &leaf,const Key &key,const Value &val){
          
        int i;
        for( i=0;i<leaf.cnt;++i)
              if(key<leaf.data[i].first) break;

          

         for(int j=leaf.cnt;j>i;--j){
            leaf.data[j].first=leaf.data[j-1].first;
            leaf.data[j].second=leaf.data[j-1].second;
         }

         leaf.data[i].first = key;
         leaf.data[i].second = val;

    ++leaf.cnt;

    ++BPTInfo.size;
  
    write_basic_information(BPTInfo,info_offset);

    //cout<<"now this leaf 's cnt is"<<leaf.cnt<<"offset is"<<leaf.offset<<endl;

    if(leaf.cnt < L) write_leafnode(leaf,leaf.offset);

    else {split_leaf(leaf);}

    return pair<iterator,OperationResult>();


      }

      void insert_node (odnode & node, const Key key ,const int insertnode){

         // cout<<"call insert_node"<<endl;
         // cout<<"node.cnt"<<node.cnt<<endl;
         // cout<<"node.offset"<<node.offset<<endl;
         int i=0;
         for(i=0;i<node.cnt;++i)
            if (key < node.key[i]) break;

         for(int j=node.cnt;j>i;--j)  
         {
             node.key[j]=node.key[j-1];
             node.child[j]=node.child[j-1];
         }

         node.key[i] =  key;
         node.child[i] = insertnode;

         ++node.cnt;

         if(node.cnt < M)  write_odnode(node,node.offset);
         else split_node(node);

      }
    

    void split_node(odnode &node){
      ///  cout<<"call split_node"<<endl;
        odnode newnode;
        leafnode leaf;
        
        newnode.cnt = node.cnt -(node.cnt/2);
        node.cnt/=2;
       

        newnode.parent = node.parent;

       
        newnode.offset=BPTInfo.eof;

         BPTInfo.eof += sizeof(odnode);

         newnode.node_type = node.node_type;
          
        for(int i=0;i<newnode.cnt;++i)
        { newnode.child[i] = node.child[i+node.cnt];
          newnode.key[i] = node.key[i+node.cnt];
        }
         
      odnode tmpod;
      if(newnode.node_type == 1){
        for(int i=0;i < newnode.cnt;++i)
        {   op.seekg(newnode.child[i]);
            op.read(reinterpret_cast<char *> (&leaf),sizeof(leafnode));
            leaf.parent = newnode.offset;
            write_leafnode(leaf,leaf.offset);

        }
        }else {
             for(int i=0;i<newnode.cnt;++i){
                 op.seekg(newnode.child[i]);
                 op.read(reinterpret_cast<char*> (&tmpod),sizeof(odnode));
                 tmpod.parent = newnode.offset;
                 write_odnode(tmpod,tmpod.offset);
             }


            
        }
//////  create a new root;
//cout<<"node.offset(if = root)"<<node.offset<<endl;

        if(node.offset == BPTInfo.root){
             odnode newroot;
             newroot.parent = 0;
             newroot.node_type = 0;
            

             newroot .offset = BPTInfo.eof; 

              BPTInfo.eof += sizeof(odnode);

             newnode.cnt=2;
            
             newroot.child[0]=node.offset;
             newroot.child[1]=newnode.offset;
             newroot.key[0]=node.key[0];
             newroot.key[1]=newnode.key[0];

             node.parent=newroot.offset;
             newnode.parent=newroot.offset;
             BPTInfo.root=newroot.offset;

             write_basic_information(BPTInfo,info_offset);
             write_odnode(node,node.offset);
             write_odnode(newnode,node.offset);
             write_odnode(newroot,newroot.offset);

             
        }else{
             write_basic_information(BPTInfo,info_offset);
             write_odnode(node,node.offset);
             write_odnode(newnode,node.offset);
             odnode par;
             op.seekg(newnode.parent );
             op.read(reinterpret_cast<char*> (&par),sizeof(odnode));
             insert_node(par,newnode.key[0],newnode.offset);
        }
        
    }

    void split_leaf(leafnode& leaf){
       // cout<<"call split leaf"<<endl;
        leafnode  newleaf;
        newleaf.cnt = leaf.cnt - leaf.cnt/2;
        leaf.cnt /= 2;
     //  cout<<"current eof is " << BPTInfo.eof<<endl;

        newleaf.offset = BPTInfo.eof;

        BPTInfo.eof += sizeof(leafnode);

        newleaf.parent = leaf.parent;

        for(int i = 0;i<newleaf.cnt;++i){
            newleaf.data[i].first = leaf.data[i+leaf.cnt].first;
            newleaf.data[i].second = leaf.data[i+leaf.cnt].second;
           
        
        }

        newleaf.next = leaf.next;
        newleaf.prev = leaf.offset;
        leaf.next = newleaf.offset;

        leafnode nextleaf;
        if(newleaf.next == 0)   BPTInfo.tail = newleaf.offset;
        else{
            op.seekg(newleaf.next);
            op.read(reinterpret_cast<char*>(&nextleaf),sizeof(leafnode));
            nextleaf.prev = newleaf.offset;

            write_leafnode(nextleaf,nextleaf.offset);
        }

        write_leafnode(leaf,leaf.offset);
        write_leafnode(newleaf,newleaf.offset);
        write_basic_information(BPTInfo,info_offset);
///////////////////////////////////////////
        leafnode l;
        op.seekg(newleaf.offset);
        op.read(reinterpret_cast<char*> (&l),sizeof(leafnode));
       // cout<<"the offset of this new leaf node is "<<l.offset<<endl;
       //////////////////////////////////////////////////////////////
       
        odnode baba;


        op.seekg(leaf.parent);
        op.read(reinterpret_cast<char*> (&baba),sizeof(odnode));
    //  cout<<"TTTTTTTTTTTTTTTTT"<<baba.cnt<<" "<<baba.child[0]<<" "<<baba.child[1]<<endl;
        insert_node(baba,newleaf.data[0].first,newleaf.offset);
       // cout<<"TTTTTTTTTTTTTTTTT"<<baba.cnt<<" "<<baba.child[0]<<" "<<baba.child[1]<<endl;

    }


      

  basicInfo BPTInfo;
      

        // Your private members go here

    public:
   
    

        

        

      

        class iterator {
               friend class BTree;
        private:

            // Your private members go here

            int offset;  ///  offset of the node
            int place;  /// the position of the leaf data int the leaf
            const BTree *from;  //  which btree is
      

        public:

            OperationResult modify(const Value& value){
                 leafnode p;
                 op.seekg(offset);
                 read(p,sizeof(leafnode));
                 p.data[place].second = value;
                 from->writeleaf(p,offset);
                 return Success;
            }

            iterator() {

                from = nullptr;
                place=offset=0;

            }

            iterator (int o,int p,BTree *f){
             place = p;
             offset = o;
             from = f;
            }

            iterator(const iterator& other) {
                from = other.from;
                offset = other.offset;
                place = other.place;
                 

            }
            // Return a new iterator which points to the n-next elements

            iterator operator++(int) {
                   iterator tmp = *this;
                   if( *this = from.end()){
                      from = nullptr;
                      place = 0;
                      offset = 0;
                      return tmp;  
                   }
                   leafnode p;
                   op.seekp(offset);
                   op.read (reinterpret_cast<char *> (&p),sizeof(leafnode));  ///get the leaf;
                   if (place = p.cnt-1) {
                           if(p.next==0)  ++place ;
                           else {
                               offset = p.next;
                               place = 0;
                           }
                   }else ++place ;
                   return tmp;
            }

            iterator& operator++() {

       
                   if( *this = from.end()){
                      from = nullptr;
                      place = 0;
                      offset = 0;
                      return *this;  
                   }
                   leafnode p;
                   op.seekp(offset);

                   op.read (reinterpret_cast<char *>(&p),sizeof(leafnode));  ///get the leaf;
                   if (place = p.cnt-1) {
                           if(p.next==0)  ++place ;
                           else {
                               offset = p.next;
                               place = 0;
                           }
                   }else ++place ;
                   return *this;

            }

            iterator operator--(int) {
                   iterator tmp;
                   if (*this == from ->begin()){
                       from = nullptr;
                       place = 0;
                       offset = 0;
                       return tmp;
                   }

                   leafnode a,b;
                  op.seekg(offset);
                   op.read(reinterpret_cast<char*> (&a),sizeof(leafnode));

                   if(a.place == 0){
                     offset = a.prev;
                     op.seekg(offset);
                     op.read (reinterpret_cast<char*> (&b),sizeof(leafnode));
                     place = b.cnt - 1;
                    
                     

                  }else --place;

           return tmp;

            }

            iterator& operator--() {

                if(*this == from -> begin()){
                     from = nullptr ;
                     place = 0;
                     offset = 0;
                     return *this ;
                }
                 leafnode a,b;
                   op.seekg(offset);
                   op.read(reinterpret_cast<char*> (&a),sizeof(leafnode));

                   if(a.place == 0){
                     offset = a.prev;
                     op.seekg(offset);
                     op.read (reinterpret_cast<char*> (&b),sizeof(leafnode));
                     place = b.cnt - 1;
                    
                     

                  }else --place;

           return *this;
                

            }

            // Overloaded of operator '==' and '!='

            // Check whether the iterators are same

            bool operator==(const iterator& rhs) const {

              return from==rhs.from&&place==rhs.place&&offset==rhs.offset;

            }

            bool operator==(const const_iterator& rhs) const {

             return from==rhs.from&&place==rhs.place&&offset==rhs.offset;

            }

            bool operator!=(const iterator& rhs) const {

               return from!=rhs.from||place!=rhs.place||offset!=rhs.offset;

            }

            bool operator!=(const const_iterator& rhs) const {

           return from!=rhs.from||place!=rhs.place||offset!=rhs.offset;

            }

        };

        class const_iterator {

            // it should has similar member method as iterator.

            //  and it should be able to construct from an iterator.
            friend class BTree;

        private:

            
            int offset;  ///  offset of the node
            int place;  /// the position of the leaf data int the leaf
            const BTree *from;  //  which btree is

        public:
     

            const_iterator() {

                from = nullptr;
                place=offset=0;

            }

             const_iterator (int o,int p,BTree *f){
             place = p;
             offset = o;
             from = f;
            }

           const_iterator(const const_iterator& other) {
                from = other.from;
                offset = other.offset;
                place = other.place;
                 

            }
             const_iterator(const iterator& other) {
                from = other.from;
                offset = other.offset;
                place = other.place;
            }
            

 const_iterator operator++(int) {
                   const_iterator tmp = *this;
                   if( *this = from.end()){
                      from = nullptr;
                      place = 0;
                      offset = 0;
                      return tmp;  
                   }
                   leafnode p;
                   op.seekp(offset);
                   op.read (reinterpret_cast<char *> (&p),sizeof(leafnode));  ///get the leaf;
                   if (place = p.cnt-1) {
                           if(p.next==0)  ++place ;
                           else {
                               offset = p.next;
                               place = 0;
                           }
                   }else ++place ;
                   return tmp;
            }

            const_iterator& operator++() {

       
                   if( *this = from.end()){
                      from = nullptr;
                      place = 0;
                      offset = 0;
                      return *this;  
                   }
                   leafnode p;
                   op.seekp(offset);

                   op.read (reinterpret_cast<char *>(&p),sizeof(leafnode));  ///get the leaf;
                   if (place = p.cnt-1) {
                           if(p.next==0)  ++place ;
                           else {
                               offset = p.next;
                               place = 0;
                           }
                   }else ++place ;
                   return *this;

            }

            const_iterator operator--(int) {
                   const_iterator tmp;
                   if (*this == from ->begin()){
                       from = nullptr;
                       place = 0;
                       offset = 0;
                       return tmp;
                   }

                   leafnode a,b;
                  op.seekg(offset);
                   op.read(reinterpret_cast<char*> (&a),sizeof(leafnode));

                   if(a.place == 0){
                     offset = a.prev;
                     op.seekg(offset);
                     op.read (reinterpret_cast<char*> (&b),sizeof(leafnode));
                     place = b.cnt - 1;
                    
                     

                  }else --place;

           return tmp;

            }

            const_iterator& operator--() {

                if(*this == from -> begin()){
                     from = nullptr ;
                     place = 0;
                     offset = 0;
                     return *this ;
                }
                 leafnode a,b;
                   op.seekg(offset);
                   op.read(reinterpret_cast<char*> (&a),sizeof(leafnode));

                   if(a.place == 0){
                     offset = a.prev;
                     op.seekg(offset);
                     op.read (reinterpret_cast<char*> (&b),sizeof(leafnode));
                     place = b.cnt - 1;
                    
                     

                  }else --place;

           return *this;
                

            }

            
            bool operator==(const iterator& rhs) const {

              return from==rhs.from&&place==rhs.place&&offset==rhs.offset;

            }

            bool operator==(const const_iterator& rhs) const {

             return from==rhs.from&&place==rhs.place&&offset==rhs.offset;

            }

            bool operator!=(const iterator& rhs) const {

               return from!=rhs.from||place!=rhs.place||offset!=rhs.offset;

            }

            bool operator!=(const const_iterator& rhs) const {

           return from!=rhs.from||place!=rhs.place||offset!=rhs.offset;

            }

        };

        // Default Constructor and Copy Constructor

        BTree() {

            op.open ("asdasdasd",std::ios::in|std::ios::out|std::ios::binary|std::ios::trunc);
            if(!op)  cout<<"open unsuccessfully !"<<endl;
										//						    else {cout<<"OPEN FILE-----"<<endl;}   
                BPTInfo.size = 0;
				BPTInfo.eof = sizeof(basicInfo);
				odnode root;
				leafnode leaf;
				BPTInfo.root = root.offset =  BPTInfo.eof;
				BPTInfo.eof += sizeof(odnode);
				BPTInfo.head =  BPTInfo.tail = leaf.offset =  BPTInfo.eof;
				BPTInfo.eof += sizeof(leafnode);
				root.parent = 0; root.cnt = 1; root.node_type = 1;
				root.child[0] = leaf.offset;
				leaf.parent = root.offset;
				leaf.next = leaf.prev = 0;
				leaf.cnt = 0;

            write_leafnode(leaf,leaf.offset);
            write_odnode(root,root.offset);
            write_basic_information(BPTInfo,info_offset);
        }

        BTree(const BTree& other) {

            // give up this section

        }

        BTree& operator=(const BTree& other) {

            // Todo Assignment

        }

        ~BTree() {

          if(op.is_open()) op.close();

        }

        // Insert: Insert certain Key-Value into the database

        // Return a pair, the first of the pair is the iterator point to the new

        // element, the second of the pair is Success if it is successfully inserted

        pair<iterator, OperationResult> insert(const Key& key, const Value& value) {
         
            int leaf_location = find_leaf(BPTInfo.root ,key);

           

            leafnode leaf;
          pair<iterator,OperationResult>  ans;
            if (BPTInfo.size == 0 || leaf_location==0){

              
           
                   op.seekg(BPTInfo.head);
                   op.read(reinterpret_cast<char*> (&leaf),sizeof(leafnode));
                   pair <iterator,OperationResult> ans = insert_leaf(leaf,key,value);
                   if (ans.second !=Success)  {return ans ;}
                   int offset = leaf.parent;
                   odnode node;
                   while(offset!=0){
                        op.seekg(offset);
                        op.read(reinterpret_cast<char*>(&node),sizeof(odnode));
                        node.key[0]=key;
                        write_odnode(node,offset);
                        offset = node.parent;

                   }
                   return ans;

                   
                
                   






            }else{
              op.seekg(leaf_location);
              op.read(reinterpret_cast<char *> (&leaf),sizeof(leafnode));
              pair<iterator,OperationResult> ans = insert_leaf(leaf,key,value);
              return ans ;
            }
 
        }

        // Erase: Erase the Key-Value

        // Return Success if it is successfully erased

        // Return Fail if the key doesn't exist in the database

        OperationResult erase(const Key& key) {

            // TODO erase function

            return Fail;  // If you can't finish erase part, just remaining here.

        }

        // Return a iterator to the beginning

        iterator begin() {return iterator(this,BPTInfo.head,0);}

        const_iterator cbegin() const {return const_iterator(this,BPTInfo.head,0);}

        // Return a iterator to the end(the next element after the last)

        iterator end() {

           leafnode t;
           op.seekg(BPTInfo.tail);
           op.read(reinterpret_cast<char*> (&t),sizeof(leafnode));
           return iterator(BPTInfo.tail,t.cnt,this);

        }

        const_iterator cend() {
                 leafnode t;
           op.seekg(BPTInfo.tail);
           
           op.read(reinterpret_cast<char*> (&t),sizeof(leafnode));
           return const_iterator(BPTInfo.tail,t.cnt,this);

        }

        // Check whether this BTree is empty

        bool empty()  {return BPTInfo.size == 0;}

        // Return the number of <K,V> pairs

        size_t size() {return BPTInfo.size;}

        // Clear the BTree

        void clear() {
            op.close();
        }

        // Return the value refer to the Key(key)

        Value at(const Key& key){
          return find(key);

             
        }

        /**

         * Returns the number of elements with key

         *   that compares equivalent to the specified argument,

         * The default method of check the equivalence is !(a < b || b > a)

         */

        size_t count(const Key& key) const{return static_cast <size_t> (find(key) != iterator(nullptr));}

        /**

         * Finds an element with key equivalent to key.

         * key value of the element to search for.

         * Iterator to an element with key equivalent to key.

         *   If no such element is found, past-the-end (see end()) iterator is

         * returned.

         */

        Value find(const Key& key) {
            int leaf_location = find_leaf(key,BPTInfo.root);
            
           
              leafnode leaf;
              op.seekg(leaf_location);
              op.read(reinterpret_cast<char*> (&leaf),sizeof(leafnode));
              for(int i=0;i<leaf.cnt;++i)
                if(leaf.data[i].first == key )  return leaf.data[i].second;

        

        }

        int root_off(){cout<<"present root's offset is"<<BPTInfo.root<<endl;}

        void debug_traverse() {
				basicInfo infoo;
                op.seekg(info_offset);
				 op.read(reinterpret_cast<char*> (&infoo),sizeof(basicInfo));
				int cur = infoo.head;
				leafnode leaf;
				std :: cout << BPTInfo.head << ' ' << BPTInfo.tail << std :: endl;
				std :: cout << "\nbegin traverse: \n";
				while(1) {
                     op.seekg(cur);
				 op.read(reinterpret_cast<char*> (&leaf),sizeof(leafnode));
                    
					
					std :: cout << "leaf size = " << leaf.cnt << std :: endl;
					for (int i=0; i<leaf.cnt; ++i) std :: cout << "(" << leaf.data[i].first << ", " << leaf.data[i].second << ")  ";
					if(cur == infoo.tail) break;
					cur = leaf.next;
				}
				std :: cout << '\n';
				std :: cout << "=====================\n";
			}

        
            /*  const_iterator find(const Key& key)  {

                  int leaf_location = find_leaf(key,BPTInfo.root);
            if(leaf_location == 0)  return cend();
            else{
              leafnode leaf;
             op.seekg(leaf_location);
             op.read(reinterpret_cast<char*> (&leaf),sizeof(leafnode));
             for(int i=0;i<leaf.cnt;++i)
                if(leaf.data[i].first == key )  return const_iterator(this,leaf_location,i);

            }
return cend();

        }*/

    };

}  // namespace sjtu
