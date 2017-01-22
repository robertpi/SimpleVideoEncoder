open System
open System.IO
open System.Drawing
open SimpleVideoEncoder

let makeVideo outPath sourceFolder =
    use video = new VideoEncoder(outPath, 933, 933)
    
    for imagePath in Directory.EnumerateFiles(sourceFolder) |> Seq.take 50   do
        let bitmap = Image.FromFile(imagePath) :?> Bitmap
        video.AddFrame(bitmap)

[<EntryPoint>]
let main argv = 
    if argv.Length < 2 then 
        printfn "useage: SimpleVideoEncoder <out path> <image source folder>"
    else
        let outPath = argv.[0]
        let sourceFolder = argv.[1]

        printfn "Doing %s %s" outPath sourceFolder

        makeVideo outPath sourceFolder

    printfn "Done!"

    0 // return an integer exit code
