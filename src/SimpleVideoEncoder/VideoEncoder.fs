namespace SimpleVideoEncoder

module Common =
    [<Literal>]
    let LibraryName = "SimpleVideoEncoder.FlatApi.dll"

module VideoFunctions =
    open System
    open System.Runtime.InteropServices

    [<DllImport(Common.LibraryName, CallingConvention = CallingConvention.Cdecl)>]
    extern IntPtr NewVideoEncoder(IntPtr filename, uint32 width, uint32 height)

    [<DllImport(Common.LibraryName, CallingConvention = CallingConvention.Cdecl)>]
    extern void AddFrame(IntPtr video, IntPtr unmanagedBuffer, uint32 bufferLength, int width, int height)

    [<DllImport(Common.LibraryName, CallingConvention = CallingConvention.Cdecl)>]
    extern void Finalize(IntPtr video)

open System
open System.Drawing
open System.Drawing.Drawing2D
open System.Drawing.Imaging
open System.Runtime.InteropServices

type VideoEncoder(filename: string, width: int, height: int) = 
    let strPtr = Marshal.StringToHGlobalUni(filename)
    let video = VideoFunctions.NewVideoEncoder(strPtr, uint32 width, uint32 height)

    let arrayOfImage (bitmap: Bitmap) =
        // if I knew why this flip was necessary, I'd be a much wiser person
        bitmap.RotateFlip(RotateFlipType.Rotate180FlipX)
        let bmpdata = bitmap.LockBits(new Rectangle(0, 0, bitmap.Width, bitmap.Height), ImageLockMode.ReadOnly, bitmap.PixelFormat)
        try
            let numbytes = bmpdata.Stride * bitmap.Height
            let bytedata: byte[] = Array.zeroCreate numbytes
            let ptr = bmpdata.Scan0;

            Marshal.Copy(ptr, bytedata, 0, numbytes)

            bytedata
        finally
            bitmap.UnlockBits(bmpdata)

    member this.AddFrame(image: Bitmap) =
        // need to ensure image is exactly same size as video
        // otherwise we'll over flow a C++ buffer
        let image =
            if image.Width <> width || image.Height <> height then
                let resizedBitmap = new Bitmap(width, height)
                use g = Graphics.FromImage(resizedBitmap)
                g.InterpolationMode <- InterpolationMode.HighQualityBicubic;
                g.DrawImage(image, 0, 0, width, height)
                resizedBitmap
            else
                image
        let buffer = arrayOfImage image
        let bufferHdl = GCHandle.Alloc(buffer, GCHandleType.Pinned)
        VideoFunctions.AddFrame(video, bufferHdl.AddrOfPinnedObject(), uint32 buffer.Length, image.Width, image.Height)
        bufferHdl.Free()

    interface IDisposable with
        member this.Dispose() =
            VideoFunctions.Finalize(video)

    member this.Dispose() =
        (this :> IDisposable).Dispose()
            