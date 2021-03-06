

//Written by Moritz Basel
// Notes: These Functions contain very few Sanity tests and no Parameter Validation.
// Passing Illegal Arguments always causes undefined behaviour
// Calling Functions in the wrong order causes Crashes








#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")


#include <d3d11.h>
#include <d3dcompiler.h>
#include <queue>


struct IndicesGroup
{
	byte index[6];
	byte padding[2];
};


ID3D11Device* m_device;
ID3D11DeviceContext* m_deviceContext;
ID3D11ComputeShader* m_computeShader;



ID3D11Buffer* m_visibilityMatrixBuffer;
ID3D11Buffer* m_indicesInputBuffer;
ID3D11Buffer* m_booleanOutputBuffer;
ID3D11Buffer* m_outputCPUBuffer;
ID3D11UnorderedAccessView* m_booleanOutputView;
ID3D11ShaderResourceView* m_indicesInputView;
ID3D11ShaderResourceView* m_visibilityMatrixView;



IndicesGroup* m_indicesPassArray;
int m_currentlyCalculatingIndicesAmount;
std::queue<IndicesGroup*> m_indicesDispatchQueue;
std::queue<IndicesGroup*> m_indicesCalculatingQueue;




extern "C" void Initialize()											//Create the device
{
	D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, 0, NULL, NULL, 6, D3D11_SDK_VERSION, &m_device, NULL, &m_deviceContext);

	m_visibilityMatrixBuffer = 0;
	m_visibilityMatrixView = 0;



	//Create and set the Shader
	ID3D10Blob* csBlob;


	D3DCompileFromFile(L"Compute.hlsl", NULL, NULL, "IsShattered", "cs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &csBlob, NULL);
	m_device->CreateComputeShader(csBlob->GetBufferPointer(), csBlob->GetBufferSize(), NULL, &m_computeShader);
	m_deviceContext->CSSetShader(m_computeShader, NULL, 0);







}






//	bytes			
extern "C" void PassVisibiltyMatrix(byte* matrix, INT64 matSize)			//Matrix is square, so size of the matrix array is size*size; needs to be made into bitfield and passed to constantbuffer 
{
	if (m_visibilityMatrixBuffer != 0)
	{
		m_visibilityMatrixBuffer->Release();
		m_visibilityMatrixBuffer = 0;

	}
	if (m_visibilityMatrixView)
	{
		m_visibilityMatrixView->Release();
		m_visibilityMatrixView = 0;
	}



	size_t size = (matSize * matSize) / 8 + 1;

	byte* matpass = new byte[size];

	for (int i = 0;i < size;i++)
	{
		byte Byte = 0;
		for (int j = 0;j < 7;j++)
		{

			Byte = Byte + (((byte)((*(matrix + i * 8 + j)) == 0) ? 0 : 1) << j);

		}
		matpass[i] = Byte;
	}


	D3D11_SUBRESOURCE_DATA matrixResourceData;
	ZeroMemory((void*)&matrixResourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	matrixResourceData.pSysMem = matpass;


	D3D11_BUFFER_DESC visibilityBufferDesc;
	ZeroMemory(&visibilityBufferDesc, sizeof(D3D11_BUFFER_DESC));
	visibilityBufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS |
		D3D11_BIND_SHADER_RESOURCE;
	visibilityBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	visibilityBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	visibilityBufferDesc.ByteWidth = size;

	m_device->CreateBuffer(&visibilityBufferDesc, &matrixResourceData, &m_visibilityMatrixBuffer);


	D3D11_SHADER_RESOURCE_VIEW_DESC visibilityViewDesc;
	ZeroMemory(&visibilityViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	visibilityViewDesc.Format = DXGI_FORMAT_UNKNOWN;
	visibilityViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	visibilityViewDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
	visibilityViewDesc.BufferEx.NumElements = size;




	m_device->CreateShaderResourceView(m_visibilityMatrixBuffer, &visibilityViewDesc, &m_visibilityMatrixView);









}



// indices_pass MUST be ordererd lowest to highest and then 0 if checking for subsets of size <6; e.g. [1,23,43,44,0,0] will check for Dimension 4 on the vertices 1,23,43,44		
//
//6 concatenated bytes
extern "C" UINT64 DispatchCheck(UINT64 indicesPass)
{
	IndicesGroup* indices = new IndicesGroup();
	for (int i = 0;i < 6;i++)
	{
		indices->index[i] = (indicesPass & (0xFF << i));			//splits the Int64 up??!! i think
	}



	m_indicesDispatchQueue.push(indices);





	return m_indicesDispatchQueue.size();
}



// DANGER! DO NOT call this method without calling result first; to flush the GPU Queue and to actually get results. Failure to do so results in UNDEF or crash
// Call this only with a indicesDispatchQueue size of amultiple of 16. This is due to Thread number constraints.


extern "C" void Compute()
{




	//reorder the data from the indicesqueue

	size_t indicesAmount = m_indicesDispatchQueue.size();
	if (indicesAmount == 0) return;


	//This codeblock is inefficient, as it creates new elements as it deletes them - it would be better to pass around pointers.
	IndicesGroup* m_indicesPassArray=new IndicesGroup[indicesAmount];
	for (int i = 0;i < indicesAmount;i++)
	{


		IndicesGroup* key = m_indicesDispatchQueue.front();



		m_indicesPassArray[i] = *key;
		m_indicesCalculatingQueue.push(key);



		m_indicesDispatchQueue.pop();
	}
	m_currentlyCalculatingIndicesAmount = indicesAmount;

	// m_indicesPassArray will be passed to the shader. It holds all indices to be checked as a IndicesGroup (size 6 bytes) each




	//m_indicesCalculatingQueue now holds the indices that are in calculation by GPU after Compute() returns. It will be used to check which Indices are a success






	//clearing these buffers for subsequent calls of compute() is responsibility of result()
	//Create the input buffer

	D3D11_BUFFER_DESC inputBufferDesc;
	ZeroMemory(&inputBufferDesc, sizeof(D3D11_BUFFER_DESC));
	inputBufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS |
		D3D11_BIND_SHADER_RESOURCE;
	inputBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	inputBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	inputBufferDesc.ByteWidth = indicesAmount * sizeof(IndicesGroup);
	inputBufferDesc.StructureByteStride=sizeof(IndicesGroup);

	D3D11_SUBRESOURCE_DATA indexData;
	ZeroMemory(&indexData, sizeof(D3D11_SUBRESOURCE_DATA));
	indexData.pSysMem = m_indicesPassArray;

	m_device->CreateBuffer(&inputBufferDesc, &indexData, &m_indicesInputBuffer);

	//Create the input Buffer View


	D3D11_SHADER_RESOURCE_VIEW_DESC inputViewDesc;
	ZeroMemory(&inputViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	inputViewDesc.Format = DXGI_FORMAT_UNKNOWN;
	inputViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	inputViewDesc.BufferEx.FirstElement=0;	
	inputViewDesc.BufferEx.NumElements=indicesAmount;






	m_device->CreateShaderResourceView(m_indicesInputBuffer, &inputViewDesc, &m_indicesInputView);



	//create the output buffer

	D3D11_BUFFER_DESC outputBufferDesc;
	ZeroMemory(&outputBufferDesc, sizeof(D3D11_BUFFER_DESC));
	outputBufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS |
		D3D11_BIND_SHADER_RESOURCE;
	outputBufferDesc.ByteWidth = indicesAmount / 8 + 1;
	outputBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	outputBufferDesc.CPUAccessFlags = 0;
	outputBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

	m_device->CreateBuffer(&outputBufferDesc, NULL, &m_booleanOutputBuffer);


	D3D11_UNORDERED_ACCESS_VIEW_DESC outputViewDesc;
	ZeroMemory(&outputViewDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
	outputViewDesc.Format = DXGI_FORMAT_UNKNOWN;
	outputViewDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
	outputViewDesc.Buffer.NumElements = indicesAmount / 8 + 1;
	outputViewDesc.Buffer.FirstElement = 0;
	outputViewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;


	m_device->CreateUnorderedAccessView(m_booleanOutputBuffer, &outputViewDesc, &m_booleanOutputView);



	ID3D11ShaderResourceView* pass[2];
	pass[0] = m_visibilityMatrixView;
	pass[1] = m_indicesInputView;

	m_deviceContext->CSSetUnorderedAccessViews(0, 1, &m_booleanOutputView, NULL);
	m_deviceContext->CSSetShaderResources(0, 2, pass);





	// dispatch??!!!

	//TODO calculate group amounts x,y,z so that x*y*z=groupAmount;

	int x;
	x = indicesAmount / (16 * 512);
	m_deviceContext->Dispatch(x,4,4);




}

//Danger! this method should only be called directly before a new Compute is called.
//returns -1 for error, -2 for no success; else an index indicating the index of vertices-set that has been found shattered.
extern "C" INT64 result()
{
	INT64 returnValue=-2;
	D3D11_BUFFER_DESC outputReadBufferDesc;
	ZeroMemory(&outputReadBufferDesc, sizeof(D3D11_BUFFER_DESC));
	outputReadBufferDesc.BindFlags = 0;
	outputReadBufferDesc.ByteWidth = m_currentlyCalculatingIndicesAmount / 8 + 1;
	outputReadBufferDesc.Usage = D3D11_USAGE_STAGING;
	outputReadBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	outputReadBufferDesc.MiscFlags = 0;

	m_device->CreateBuffer(&outputReadBufferDesc, NULL, &m_outputCPUBuffer);

	m_deviceContext->CopyResource(m_outputCPUBuffer, m_booleanOutputBuffer);


	D3D11_MAPPED_SUBRESOURCE mapped;

	m_deviceContext->Map(m_outputCPUBuffer, 0, D3D11_MAP_READ, 0, &mapped);

	int success = 0;
	int pos = 8;
	int byteOffset = -1;
	for (int i = 0;i < m_currentlyCalculatingIndicesAmount / 8 + 1;i++)
	{

		if (*((byte*)(mapped.pData)) + i != 0)
		{
			success = 1;
			byteOffset = i;
			for (int j = 0;j < 7;j++)
			{
				if ((1 << j) & (*((byte*)(mapped.pData)) + i))
				{
					pos = j;

				}
			}
		}
	}

	m_deviceContext->Unmap(m_outputCPUBuffer, 0);
	if (success == 1)
	{
		if (pos == 8) return -1;
		return byteOffset * 8 + pos;

	}

	//Freeing the array passed in Compute()

	delete [] m_indicesPassArray;



	//Clearing stuff out



	m_currentlyCalculatingIndicesAmount = 0;

	m_indicesInputBuffer->Release();
	m_indicesInputBuffer = 0;
	m_booleanOutputBuffer->Release();
	m_booleanOutputBuffer = 0;
	m_booleanOutputView->Release();
	m_booleanOutputView = 0;
	m_outputCPUBuffer->Release();
	m_outputCPUBuffer = 0;
	m_indicesInputView->Release();
	m_indicesInputView = 0;

	return -1;




}

//returns 6 byte denoting the indices of vertices, leading 2 byte unused - do not call without having called result() first. Otherwise, causes UNDEF
extern "C" UINT64 GetIndicesByIndex(INT64 index)
{
	IndicesGroup* indices=0;
	UINT64 indicesReturn = 0;
	for (int i = 0;i < m_currentlyCalculatingIndicesAmount;i++)
	{
		if (index == i)
			indices = m_indicesCalculatingQueue.front();
		if (index != i)
			delete m_indicesCalculatingQueue.front();
		m_indicesCalculatingQueue.pop();
	}
	for (int i = 0;i < 6;i++)
	{
		indicesReturn += indices->index[i] << (8 * i);				//Should work; concatenates 6 byte

	}
	return indicesReturn;

}




extern "C" void Cleanup()												//Duh
{



	m_device->Release();
	m_deviceContext->Release();
	m_visibilityMatrixBuffer->Release();
	m_computeShader->Release();



}


