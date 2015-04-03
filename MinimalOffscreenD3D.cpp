
#define WIN32_LEAN_AND_MEAN

#include <d3d11.h>
#pragma comment(lib, "d3d11")

#include <wincodec.h> // do zapisu obrazu: Windows Imaging Components (WIC)
#pragma comment(lib, "windowscodecs")

#include <atlbase.h> // CComPtr u³atwi pracê ze wskaŸnikami (RAII)

// binary_t pomocnik do ³adowania zasobów binarnych z dysku
#include <vector>
#include <fstream>  // ifstream
#include <iterator> // istream_iterator
struct binary_t : std::vector<unsigned char>
{
	binary_t(const char path[]) : vector( // konstruktor przyjmuje œcie¿kê do pliku binarnego
		std::istream_iterator<value_type>(std::ifstream(path, std::ios::binary) >> std::noskipws),
		std::istream_iterator<value_type>())
	{}
};


int main()
{
	static const UINT IMAGE_WIDTH=640, IMAGE_HEIGHT=480, NUM_POINTS=4;

	// tworzymy urz¹dzenie i kontekst Direct3D
	// - ¿eby wiedzieæ co siê dzieje na karcie graficznej, mo¿na w³¹czyæ: 
	//   Debug -> Graphics -> DirectX Control Panel -> Debug Layer -> Force On (wymusiæ warstwê debug)

	CComPtr<ID3D11Device> dev; 
	CComPtr<ID3D11DeviceContext> ctx;
	::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, 0, nullptr, 0, D3D11_SDK_VERSION, &dev, 0, &ctx);

	CComPtr<ID3D11Texture2D> target, image; // tworzymy teksturê wynikow¹ i obraz wyjœciowy
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width  = IMAGE_WIDTH;                 // wymiary obrazka
	desc.Height = IMAGE_HEIGHT;
	desc.ArraySize = 1;                        // trzeba ustawiæ (trzeci wymiar tekstury?)...
	desc.SampleDesc.Count = 1;                 // wielokrotne próbkowanie
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // albo np. DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET; // do czego bêdziemy potrzebowaæ tej tekstury
	dev->CreateTexture2D(&desc, nullptr, &target);

	desc.BindFlags = 0;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	dev->CreateTexture2D(&desc, nullptr, &image);


	// tworzymy widok na teksturê i ustawiamy jako "wyjœcie" z shadera

	CComPtr<ID3D11RenderTargetView> view;
	dev->CreateRenderTargetView(target, nullptr, &view); // nie podajemy D3D11_RENDER_TARGET_VIEW_DESC, wymiary bêd¹ skopiowane z tekstury
	ctx->OMSetRenderTargets(1, &view.p, nullptr); // jeœli potrzebujemy bufor g³êbi to trzeba jeszcze ustawiæ ID3D11DepthStencilState

	// ustawiamy viewport

	D3D11_VIEWPORT viewport = {};
	viewport.Width  = IMAGE_WIDTH;
	viewport.Height = IMAGE_HEIGHT;
	viewport.MaxDepth = D3D11_MAX_DEPTH; // trzeba ustawiæ
	ctx->RSSetViewports(1, &viewport);

	// wczytujemy i ustawiamy shadery
	// - mo¿emy kompilowaæ je ze Ÿróde³ ::D3D10CompileShader(), co wymaga³oby linkowania z d3d10.lib
	// - VS od 2012 pozwala na budowanie shaderów, mo¿na ustawiæ tworzenie nag³ówka z wykonywalnym kodem w postaci tablicy znaków
	// - tutaj ³adujemy do pamiêci kod skompilowanego shadera z pliku *.cso (compiled shader object) i tworzymy shader

	CComPtr<ID3D11VertexShader> vertex_shader;
	const binary_t vertex_code = "VertexShader.cso";
	dev->CreateVertexShader(vertex_code.data(), vertex_code.size(), nullptr, &vertex_shader);
	ctx->VSSetShader(vertex_shader, nullptr, 0);


	CComPtr<ID3D11PixelShader>  pixel_shader;
	const binary_t pixel_code = "PixelShader.cso";
	dev->CreatePixelShader(pixel_code.data(), pixel_code.size(), nullptr, &pixel_shader);
	ctx->PSSetShader(pixel_shader, nullptr, 0);

	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP); // bêdziemy rysowaæ trójk¹ty "ciurkiem"
	
	// rysujemy

	ctx->Draw(NUM_POINTS, 0); // rysujemy cztery wierzcho³ki (bez danych wejœciowych, shader wyliczy wspó³rzêdne)

	// sp³ukujemy :)

	ctx->Flush(); // to jest wa¿ne, zamiast IDXGISwapchain::Present();

	// kopiujemy piksele z tekstury wynikowej do obrazu, z którego mo¿emy je odczytaæ

	ctx->CopyResource(image, target); // tu siê lepiej nie pomyliæ

	// mapujemy zasób do pamiêci, ¿eby dostaæ siê do pikseli obrazu - WA¯NE ¿eby potem j¹ odmapowaæ!
	D3D11_MAPPED_SUBRESOURCE resource = {};
	static const UINT resource_id = D3D11CalcSubresource(0, 0, 0);
	ctx->Map(image, resource_id, D3D11_MAP_READ, 0, &resource);
	resource.pData;

	// tworzymy fabrykê WIC i bitmapê ze zmapowanej pamiêci

	CoInitialize(nullptr); // wa¿ne, ¿eby zainicializowaæ COM
	CComPtr<IWICImagingFactory> factory;
	factory.CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER);

	CComPtr<IWICBitmap> bitmap;
	factory->CreateBitmapFromMemory(
		IMAGE_WIDTH, IMAGE_HEIGHT, 
		GUID_WICPixelFormat32bppRGBA, resource.RowPitch, resource.DepthPitch, 
		(BYTE*)resource.pData, &bitmap);
	ctx->Unmap(image, resource_id); // nie zapomnijmy odmapowaæ zasobu

	// inicjalizujemy enkoder i strumieñ PNG

	CComPtr<IWICStream> stream;
	CComPtr<IWICBitmapEncoder> encoder;
	CComPtr<IWICBitmapFrameEncode> frame;

	factory->CreateStream(&stream);
	stream->InitializeFromFilename(L"image.png", GENERIC_WRITE);

	factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
	encoder->Initialize(stream, WICBitmapEncoderNoCache);
	encoder->CreateNewFrame(&frame, nullptr);
	frame->Initialize(nullptr);

	frame->WriteSource(bitmap, nullptr); // nie przekazujemy WICRect, zapisany bêdzie ca³y obraz

	frame->Commit();
	encoder->Commit();

	// wypada³oby deinicjalizowaæ COM, ale trzeba wczeœniej zniszczyæ wszystkie obiekty WIC
	//CoUninitialize();

	return 0;
}

