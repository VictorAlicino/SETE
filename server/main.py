"""Server App"""
import fastapi
import uvicorn

app = fastapi.FastAPI()

@app.post("/test")
async def test(message: str):
    print(message)
    return fastapi.status.HTTP_200_OK

def _main() -> int:
    print("Hello World")
    uvicorn.run(
        "main:app",
        host="0.0.0.0",
        port=7986,
        reload=True
    )

if __name__ == "__main__":
    _main()
