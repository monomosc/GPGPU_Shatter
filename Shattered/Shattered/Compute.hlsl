


struct IndexGroup
{
	uint index[2];		// this is annoying. I am passing 6 bytes, but hlsl does not support types smaller than 4 byte. meaning the upper 2 bytes of index[1] are 0
};




ByteAddressBuffer visibilityMatrix : register(t0);
StructuredBuffer<IndexGroup> indicesFull : register(t1);

RWByteAddressBuffer booleanOutput : register(u0);


uint AccessIndex(IndexGroup indexIn, uint slot)
{
	uint returnValue = 0;
	uint arrayIndex = 0;
	if (slot & 4)
		arrayIndex = 1;
	return (indexIn.index[arrayIndex] & (0xFF << ((slot - arrayIndex << 2) * 8)) >> ((slot - arrayIndex << 2) * 8));

}




[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_GroupThreadID, uint3 GroupID : SV_GroupID)
{
	
	


	uint polySize = 0;
	visibilityMatrix.GetDimensions(polySize);
	polySize = trunc(sqrt(polySize * 8));





	uint amount = 0;

	IndexGroup indices = indicesFull[GroupID.x * 4 * 4 * 8 * 8 * 8 + GroupID.y * 4 * 8 * 8 * 8 + GroupID.z * 8 * 8 * 8 + DTid.x * 8 * 8 + DTid.y * 8 + DTid.z]; 


	for (uint k = 0;k < 6;k++)
	{
		if (AccessIndex(indices, k) != 0) amount++;
	}

	if (AccessIndex(indices, 0) == 0) amount++;

	uint visibility_p[2];					//these two are 64 = 2^6 bits - making it the largest necessary container for a bitfield containing data. VERY UGLY
	visibility_p[0] = 0;
	visibility_p[1] = 0;

	for (uint i = 0;i < polySize;i++)				// TODO::::: IMPORTANT:::: For amounts < 6 not all the entries in visibility_p are set to 1
	{												// either: do by hand, or change detection, so missing 1s are calculated for
		uint idx = 0;
		for (uint j = 0;j < amount;j++)
		{

			//fake 2-dimensional array
			uint bit = i * polySize + AccessIndex(indices, j);		//the bit-index - i is the row, j the column of polySize*polySize Matrix




			//how to get from bit number to value:
			uint visibilityValue = visibilityMatrix.Load(trunc(bit / 8)) & (1 << (bit % 8));		

			if (visibilityValue == 1)
				idx = idx | (1 << j);
		}
		uint left_or_right;
		if (idx >= 32)
			left_or_right = 1;
		else
			left_or_right = 0;
		uint bit = idx - 32 * left_or_right;

		visibility_p[left_or_right] = visibility_p[left_or_right] | (1 << bit);





	



	}




	if (visibility_p[0] == 0xFFFFFFFF && visibility_p[1] == 0xFFFFFFFF)
	{
		uint groupBlock = GroupID.x*4*4 * 64 + GroupID.y*4 * 64 + GroupID.z * 64;			// (DONE; see IsShattered.cpp::Compute() Dispatch() call ////		apply correct NUMY and NUMZ values 
		booleanOutput.InterlockedOr(groupBlock + DTid.x * 8 + ((DTid.y & 4) >> 1), (1 << (DTid.z + 8 * (DTid.y & 3))));		// there might be an issue with byte ordering; since uint is 4 long and i can onl

	}











}







