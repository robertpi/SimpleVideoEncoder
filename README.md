
# SimpleVideoEncoder

Create mp4 videos from a series of images.

The
``` F#
let makeVideo outPath sourceFolder =
	use video = new VideoEncoder(outPath, 933, 933)
    
	for imagePath in Directory.EnumerateFiles(sourceFolder) |> Seq.take 50   do
		let bitmap = Image.FromFile(imagePath) :?> Bitmap
		video.AddFrame(bitmap)
```

