/*
 * Aptr_tester.cpp
 *
 *  Created on: Jul 17, 2016
 *      Author: user
 */

#include "Aptr_tester.h"
#include "Aptr.h"
#include "samples.h"
#include "mem.h"

int test_Aptr_create()
{
	int* tmp_ptr = (int* )memsys5Malloc(64*sizeof(int));
	Aptr<int> ptr(tmp_ptr, 64, 0);
	bool res = ptr.m_base_page_index > 0;
	memsys5Free(tmp_ptr);

	return res;
}
int test_Aptr_destroy()
{
	bool res = true;
	int* tmp_ptr = (int* )memsys5Malloc(64*sizeof(int));
	Aptr<int>* ptr = new Aptr<int>(tmp_ptr, 64, 0);
	res &= ptr->m_base_page_index > 0;

	delete ptr;
	memsys5Free(tmp_ptr);

	return res;
}
int test_Aptr_dereference()
{
	bool res = true;
	int* tmp_ptr = (int* )memsys5Malloc(64*sizeof(int));

	Aptr<int> ptr(tmp_ptr, 64, 0);

	res &= ptr.m_base_page_index > 0;

	*ptr = 1;


	res &= ptr.m_aptr.prm_ptr != NULL;
	res &= (*ptr == 1);

	memsys5Free(tmp_ptr);

	return res;
}
int test_Aptr_operator_add()
{
	bool res = true;
	int* tmp_ptr = (int* )memsys5Malloc(64*sizeof(int));

	Aptr<int> ptr(tmp_ptr, 64, 0);
	res &= ptr.m_base_page_index > 0;
	*ptr = 1;
	res &= ptr.m_aptr.prm_ptr != NULL;
	res &= (*ptr == 1);

	int old_page_offset = ptr.m_aptr.offset;

	ptr++;
	res &= ptr.m_aptr.prm_ptr != NULL;
	res &= ptr.m_aptr.offset - old_page_offset == sizeof(int);
	res &= (*ptr != 1);

	memsys5Free(tmp_ptr);

	return res;
}
int test_Aptr_operator_sub()
{
	bool res = true;
	int* tmp_ptr = (int* )memsys5Malloc(64*sizeof(int));

	Aptr<int> ptr(tmp_ptr ,64, 0);

	res &= ptr.m_base_page_index > 0;
	*ptr = 1;
	res &= ptr.m_aptr.prm_ptr != NULL;
	res &= (*ptr == 1);

	int old_page_offset = ptr.m_aptr.offset;

	ptr++;
	res &= ptr.m_aptr.prm_ptr != NULL;
	res &= ptr.m_aptr.offset - old_page_offset == sizeof(int);
	res &= (*ptr != 1);

	ptr--;
	res &= ptr.m_aptr.prm_ptr != NULL;
	res &= ptr.m_aptr.offset - old_page_offset == 0;
	res &= (*ptr == 1);

	memsys5Free(tmp_ptr);

	return res;
}

int test_Aptr_double_dereference()
{
	bool res = true;
	int* tmp_ptr = (int* )memsys5Malloc(64*sizeof(int));

	Aptr<int> ptr(tmp_ptr, 64, 0);
	*ptr = 1;
	res &= (*ptr == 1);

	res &= (*ptr == 1);

	memsys5Free(tmp_ptr);

	return res;

}
int test_Aptr_dereference_add_dereference_no_overflow()
{
	return 1;
}
int test_Aptr_dereference_add_dereference_with_overflow()
{
	bool res = true;

	int* tmp_ptr = (int* )memsys5Malloc(1500*sizeof(int));

	Aptr<int> ptr(tmp_ptr ,1500, 0);

	*ptr = 1;
	res &= (*ptr == 1);

	ptr += 1025; // should now overflow to another page

	res &= ptr.m_aptr.prm_ptr != NULL;
	res &= ptr.m_aptr.offset > PAGE_SIZE;

	return res;

	*ptr = 1;

	res &= (*ptr == 1);
	res &= ptr.m_aptr.prm_ptr != NULL;

	memsys5Free(tmp_ptr);

	return res;
}
int test_Aptr_dereference_add_dereference_with_underflow()
{
	bool res = true;

	int* tmp_ptr = (int* )memsys5Malloc(1500*sizeof(int));

	Aptr<int> ptr(tmp_ptr, 1500, 1);

	*ptr = 2;
	res &= (*ptr == 2);

	ptr += 1025; // should now overflow to another page

	res &= ptr.m_aptr.prm_ptr != NULL;
	res &= ptr.m_aptr.offset > PAGE_SIZE;

	*ptr = 1;
	res &= (*ptr == 1);
	res &= ptr.m_aptr.prm_ptr != NULL;

	ptr -= 10; // should now underflow to another page

	res &= ptr.m_aptr.prm_ptr != NULL;
	res &= ptr.m_aptr.offset < 0;

	ptr -= 1015; // should now go back to basic ptr
	res &= ptr.m_aptr.prm_ptr != NULL;
	res &= ptr.m_aptr.offset < 0;

	res &= (*ptr ==2);
	res &= ptr.m_aptr.prm_ptr != NULL;

	memsys5Free(tmp_ptr);

	return res;
}

int test_assignment_operation()
{
	bool res = true;

	int* tmp_ptr1 = (int* )memsys5Malloc(64*sizeof(int));
	int* tmp_ptr2 = (int* )memsys5Malloc(32*sizeof(int));
	
	Aptr<int> ptr1(tmp_ptr1, 64, 1);
	*ptr1 = 1;
	res &= (*ptr1 == 1);

	Aptr<int> ptr2(tmp_ptr2, 32, 1);
	*ptr2 = 2;
	res &= (*ptr2 == 2);

	res &= ptr1.m_aptr.prm_ptr == ptr2.m_aptr.prm_ptr;

	ptr2 = ptr1;

	res &= ptr2 == ptr1;
	//res &= ptr2.m_aptr.prm_ptr == NULL;
	res &= (*ptr2 == 1);

	memsys5Free(tmp_ptr1);
	memsys5Free(tmp_ptr2);

	return res;
}

int test_evict_due_to_unlinked_state()
{
        bool res = true;

        int* tmp_ptr1 = (int* )memsys5Malloc(1024*sizeof(int));
        int* tmp_ptr2 = (int* )memsys5Malloc(1024*sizeof(int));
        Aptr<int>* ptr1 = new Aptr<int>(tmp_ptr1, 1024, 1);
	ptr1->m_aptr.hint_write = true;
        int* val = (int*)deref(&ptr1->m_aptr, ptr1->m_base_page_index);
	for (int i=0;i<1024;i++)
	{
		val[i]=i;
	}

	delete ptr1;

	// Now should have room for second ptr for sure.

        Aptr<int>* ptr2 = new Aptr<int>(tmp_ptr2, 1024, 1);

        int* val2 = (int*)deref(&ptr2->m_aptr, ptr2->m_base_page_index);

	for (int i=0;i<1024;i++)
        {
                val2[i]=0;
        }

	delete ptr2;

	ptr1 = new Aptr<int>(tmp_ptr1, 1024, 1);
        int* val1 = (int*)deref(&ptr1->m_aptr, ptr1->m_base_page_index);
        for (int i=0;i<1024;i++)
        {
                res &= val1[i] == i;
		//printf("%d\n", val1[i]);
        }

        delete ptr1;


        memsys5Free(tmp_ptr1);
        memsys5Free(tmp_ptr2);

        return res;
}

int test_set_to_null()
{
	//Aptr<int> k(0,0);

	return true;
}
