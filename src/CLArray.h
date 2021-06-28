/* date = March 23rd 2021 8:17 pm */

#ifndef CL_ARRAY_H
#define CL_ARRAY_H

template<typename T>
struct CLArray {
    int count;
    int capacity;
    T* data;
    
    inline T& operator[](int i) { assert(i >= 0 && i < count); return data[i]; }
    
    inline void Resize(int newSize) {
        if(capacity >= newSize)
            return;
        
        int dblSize = capacity * 2;
        int actualSize = dblSize > newSize ? dblSize : newSize;
        
        T* newData = (T*) malloc(actualSize * sizeof(T));
        assert(newData);
        
        if(data) {
            memcpy(newData, data, sizeof(T) * count);
            free(data);
        }
        
        capacity = actualSize;
        data = newData;
    }
    
    inline void Add(const T& n) {
        if(count >= capacity) {
            Resize(count + 1);
        }
        
        memcpy(&data[count], &n, sizeof(n));
        count++;
    }
    
    inline void AddEmpty() {
        if(count >= capacity) {
            Resize(count + 1);
        }
        
        memset(&data[count], 0, sizeof(T));
        count++;
    }
    
    inline void RemoveAt(int index) {
        assert(index < count);
        
        if(index == count - 1) {
            count--;
            return;
        }
        
        int delta = count - index - 1;
        memmove(&data[index], &data[index + 1], sizeof(T) * delta);
        
        count--;
    }
    
    inline void Free() {
        if(data)
            free(data);
        
        count = 0;
        capacity = 0;
        data = NULL;
    }
    
    inline void Clear() {
        // TODO: Maybe mem set to zero leftover data? 
        count = 0;
    }
};


#endif //_C_L_ARRAY_H
