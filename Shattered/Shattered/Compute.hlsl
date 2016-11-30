



ByteAddressBuffer visibilityMatrix : register(t0);
ByteAddressBuffer indicesFull : register(t1);

ByteAddressBuffer booleanOutput : register(u0);



[numthreads(16, 16, 4)]
void main(uint3 DTid : SV_GroupThreadID, uint3 GroupId : SV_GroupID)
{
	uint pos = SV_GroupID * 8192 + DTid.x * 512 + DTid.y * 32 + DTid.z * 8;
	uint indices[6];

	uint polySize = 0;
	visibilityMatrix.GetDimensions(polySize);
	polySize = trunc(sqr(polySize * 8));





	uint amount = 0;
	for (uint i = 0;i < 6;i++)
	{
		indices[i] = indicesFull[pos + i];
		if (indices[i] != 0) amount++;
	}
	if (indices[0] == 0) amount += 1;

	uint visibility_p[2];//these two are 64 = 2^6 bits - making it the largest necessary container for a bitfield containing data. VERY UGLY
	visibility_p[0] = 0;
	visibility_p[1] = 0;

	for (uint i = 0;i < polySize;i++)				// TODO::::: IMPORTANT:::: For amounts < 6 not all the entries in visibility_p are set to 1
	{												// either: do by hand, or change detection, so missing 1s are calculated for
		uint idx = 0;
		for (uint j = 0;j < amount;j++)
		{

			//fake 2-dimensional array
			uint bit = i * 6 + indices[j];

			//how to get from bit number to value:
			uint visibilityValue = visibilityMatrix[trunc(bit / 8)] & (1 << (bit % 8);

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





		if (visibility_p[0] == 4294967295 && visibility_p[1] == 4294967295)
		{
			bit = DTid.x * 16 * 4 + DTid.y * 4 + DTid.z;

			booleanOutput[trunc(bit / 8)] = booleanOutput[trunc(bit / 8)] | (1 << (bit % 8))
		}



	}















}







