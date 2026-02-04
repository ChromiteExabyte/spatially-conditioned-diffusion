import numpy as np
import rasterio
from PIL import Image
from pathlib import Path

# Create directories
Path("data/rasters").mkdir(parents=True, exist_ok=True)
Path("data/outputs/images").mkdir(parents=True, exist_ok=True)

# 1. Create a "Ground Truth" Mask (A simple white square on black)
mask = np.zeros((1024, 1024), dtype=np.uint8)
mask[256:768, 256:768] = 255

with rasterio.open("data/rasters/site_01.tif", "w", driver="GTiff", height=1024, width=1024, count=1, dtype='uint8') as ds:
    ds.write(mask, 1)

# 2. Create an "AI Image" (The same square but slightly "shaky" or blurred)
ai_img = np.zeros((1024, 1024, 3), dtype=np.uint8)
# We make the AI slightly "off" to test the metric
ai_img[258:770, 258:770, :] = 200 
Image.fromarray(ai_img).save("data/outputs/images/site_01_v1.png")

print("✅ Fake data created. Now run: python scripts/05_batch_evaluate.py")
