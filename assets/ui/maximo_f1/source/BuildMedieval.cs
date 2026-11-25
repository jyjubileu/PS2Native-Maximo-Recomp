using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

public static class BuildMedieval
{
    private sealed class Part
    {
        public readonly string Id, Purpose, Shape;
        public readonly int W, H, XSlice, YSlice;
        public readonly double Lit;
        public Part(string id, int w, int h, int x, int y, string shape, double lit, string purpose) {
            Id=id; W=w; H=h; XSlice=x; YSlice=y; Shape=shape; Lit=lit; Purpose=purpose;
        }
    }

    private static readonly Part[] Parts = {
        new Part("panel_main", 640, 512, 104, 104, "frame", .20, "Moldura externa de bronze, couro e cantos de acanto"),
        new Part("panel_header", 768, 152, 140, 0, "header", .26, "Placa arqueada do título e do perfil"),
        new Part("panel_content", 512, 352, 26, 26, "content", .04, "Bandeja da lista"),
        new Part("panel_description", 512, 74, 34, 21, "notch", .19, "Placa da ajuda contextual"),
        new Part("tab_normal", 256, 68, 54, 21, "tab", .20, "Aba inativa"),
        new Part("tab_selected", 256, 68, 54, 21, "tab", .95, "Aba ativa"),
        new Part("row_normal", 512, 60, 24, 18, "row", .08, "Linha comum com separador rebaixado"),
        new Part("row_selected", 512, 60, 24, 18, "row", 1.0, "Linha selecionada"),
        new Part("button_normal", 256, 76, 49, 23, "button", .34, "Botão de repouso"),
        new Part("button_hover", 256, 76, 49, 23, "button", .97, "Botão apontado ou ação principal"),
        new Part("button_pressed", 256, 76, 49, 23, "button", .70, "Botão pressionado"),
        new Part("arrow_left", 48, 48, 0, 0, "left", .8, "Reduz valor ou cicla para trás"),
        new Part("arrow_right", 48, 48, 0, 0, "right", .8, "Aumenta valor ou cicla para frente"),
        new Part("scrollbar_track", 26, 256, 0, 15, "track", .40, "Trilho vertical"),
        new Part("scrollbar_thumb", 20, 144, 0, 9, "thumb", .92, "Indicador proporcional da rolagem"),
        new Part("divider_gold", 512, 16, 24, 0, "divider", .67, "Divisor horizontal com remates"),
        new Part("crest_gold", 256, 256, 0, 0, "crest", .0, "Medalhão heráldico"),
        new Part("selection_glow", 512, 72, 32, 0, "glow", 1.0, "Halo sem interior para realçar a linha")
    };

    private static Color Rgb(int r, int g, int b) { return Color.FromArgb(r,g,b); }
    private static int Clamp(int n) { return Math.Min(255, Math.Max(0, n)); }
    private static Color Mix(Color a, Color b, double t) {
        return Rgb((int)(a.R+(b.R-a.R)*t), (int)(a.G+(b.G-a.G)*t), (int)(a.B+(b.B-a.B)*t));
    }
    private static RectangleF Inset(RectangleF r, float d) {
        return new RectangleF(r.X+d,r.Y+d,r.Width-2*d,r.Height-2*d);
    }

    private static GraphicsPath Outline(string shape, RectangleF r) {
        float x=r.X, y=r.Y, w=r.Width, h=r.Height, right=r.Right, bottom=r.Bottom;
        var p=new GraphicsPath();
        if (shape=="header") {
            float cap=Math.Min(w*.21f, 112);
            p.StartFigure();
            p.AddBezier(x, bottom-9, x+cap*.59f, bottom-14, x+cap*.74f, y+32, x+cap*.13f, y+29);
            p.AddBezier(x+cap*.13f, y+29, x+w*.24f, y-6, right-w*.24f, y-6, right-cap*.13f, y+29);
            p.AddBezier(right-cap*.13f, y+29, right-cap*.74f, y+32, right-cap*.59f, bottom-14, right, bottom-9);
            p.AddLine(right,bottom-9,right-15,bottom);
            p.AddLine(right-15,bottom,x+15,bottom);
            p.CloseFigure();
            return p;
        }
        float c=shape=="frame"?28:shape=="button"?Math.Min(22,h*.31f):shape=="tab"?Math.Min(21,h*.39f):
            shape=="row"?Math.Min(12,h*.25f):shape=="track"||shape=="thumb"?w*.40f:shape=="divider"?5:
            Math.Min(16,Math.Min(w,h)*.21f);
        if (shape=="tab") {
            p.AddBezier(x+3,bottom, x+13,bottom-5, x+c-5,y, x+c+18,y);
            p.AddLine(x+c+18,y,right-c-18,y);
            p.AddBezier(right-c-18,y, right-c+5,y, right-13,bottom-5, right-3,bottom);
            p.CloseFigure();
            return p;
        }
        if (shape=="notch" || shape=="button") {
            p.AddLines(new []{new PointF(x+c,y),new PointF(right-c,y)});
            p.AddBezier(right-c,y,right-c+3,y+7,right-7,y+c-3,right,y+c);
            p.AddLine(right,y+c,right,bottom-c);
            p.AddBezier(right,bottom-c,right-7,bottom-c+3,right-c+3,bottom-7,right-c,bottom);
            p.AddLine(right-c,bottom,x+c,bottom);
            p.AddBezier(x+c,bottom,x+c-3,bottom-7,x+7,bottom-c+3,x,bottom-c);
            p.AddLine(x,bottom-c,x,y+c);
            p.AddBezier(x,y+c,x+7,y+c-3,x+c-3,y+7,x+c,y);
            p.CloseFigure();
            return p;
        }
        p.AddPolygon(new []{new PointF(x+c,y),new PointF(right-c,y),new PointF(right,y+c),
          new PointF(right,bottom-c),new PointF(right-c,bottom),new PointF(x+c,bottom),
          new PointF(x,bottom-c),new PointF(x,y+c)});
        return p;
    }

    private static uint Hash(int x,int y,uint seed) {
        unchecked {
            uint a=(uint)x*0x632BE59Bu ^ (uint)y*0x85157AF5u ^ seed*0x9E3779B9u;
            a=(a^(a>>16))*0x7FEB352Du;
            a=(a^(a>>15))*0x846CA68Bu;
            return a^(a>>16);
        }
    }
    private static double Grain(int x,int y,uint salt) { return (Hash(x,y,salt)&65535)/65535.0; }
    private static double Noise(double x,double y,uint salt) {
        int a=(int)Math.Floor(x), b=(int)Math.Floor(y);
        double px=x-a, py=y-b;
        px=px*px*(3-2*px); py=py*py*(3-2*py);
        double t=(1-px)*Grain(a,b,salt)+px*Grain(a+1,b,salt);
        double u=(1-px)*Grain(a,b+1,salt)+px*Grain(a+1,b+1,salt);
        return t+(u-t)*py;
    }

    private static Bitmap Leather(int w,int h,double lit,bool selected) {
        var tex=new Bitmap(w,h,PixelFormat.Format32bppArgb);
        var bounds=new Rectangle(0,0,w,h);
        var data=tex.LockBits(bounds, ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
        try {
            var pixels=new byte[data.Stride*h];
            for(int y=0;y<h;y++) for(int x=0;x<w;x++) {
                double v=y/(double)Math.Max(1,h-1);
                double u=x/(double)Math.Max(1,w-1);
                double n=Noise((x+y*.48)/19.0,(y-x*.15)/17.0,3);
                double grain=Noise(x/5.2,y/4.4,13)*9+(Grain(x,y,15)-.5)*9;
                double cells=Noise((x+y*.7)/4.9,(20+y-x*.22)/4.1,23);
                double fibre=(Math.Abs(cells-.5)*-16)+Math.Max(0, .11-Math.Abs(cells-.51))*42;
                double shade=14+(1-v)*7 + (n-.44)*12 + grain + fibre - 4*Math.Abs(u-.49);
                double gold=selected ? (23 + 35*Math.Pow(Math.Abs(2*v-1),2)) : 0;
                int off=y*data.Stride+x*4;
                pixels[off]=(byte)Clamp((int)(shade*.66+2+lit*4+gold*.18));
                pixels[off+1]=(byte)Clamp((int)(shade*.90+2+lit*7+gold*.52));
                pixels[off+2]=(byte)Clamp((int)(shade*1.17+5+lit*12+gold));
                pixels[off+3]=255;
            }
            Marshal.Copy(pixels,0,data.Scan0,pixels.Length);
        } finally { tex.UnlockBits(data); }
        using(var g=Graphics.FromImage(tex)) {
            g.SmoothingMode=SmoothingMode.AntiAlias;
            using (var gleam=new Pen(Color.FromArgb(20,157,107,68),.7f))
            using (var fissure=new Pen(Color.FromArgb(24,13,6,4),.8f)) {
                for(int i=0;i<w*h/200;i++) {
                    float x=(float)(Grain(i,42,29)*w), y=(float)(Grain(i,8,2)*h);
                    float l=2+(float)Grain(i,9,7)*11;
                    g.DrawLine(fissure,x,y,x+l,y+l*.34f);
                    g.DrawLine(gleam,x,y+1,x+l,y+l*.34f+1);
                }
            }
        }
        return tex;
    }

    private static LinearGradientBrush Metal(RectangleF r,double lit,bool reversed=false) {
        var brush=new LinearGradientBrush(r, Color.Black, Color.White, reversed ? 265f : 79f);
        Color[] dark={Rgb(88,47,24),Rgb(193,136,74),Rgb(111,63,33),Rgb(220,163,90),
                      Rgb(112,59,28),Rgb(155,95,40),Rgb(59,27,13)};
        Color[] gold={Rgb(174,91,21),Rgb(255,236,158),Rgb(187,113,34),Rgb(255,215,101),
                      Rgb(183,96,17),Rgb(252,183,63),Rgb(115,56,11)};
        var stops=new Color[dark.Length];
        for(int i=0;i<dark.Length;i++) stops[i]=Mix(dark[i],gold[i],lit);
        brush.InterpolationColors=new ColorBlend {Colors=stops,Positions=new float[]{0,.12f,.28f,.42f,.64f,.83f,1}};
        return brush;
    }

    private static void FillOutline(Graphics g,string style,RectangleF r,Color color) {
        using(var shape=Outline(style,r)) using(var fill=new SolidBrush(color)) g.FillPath(fill,shape);
    }
    private static void Brass(Graphics g,string style,RectangleF r,double lit,bool reverse=false) {
        using(var shape=Outline(style,r)) using(var fill=Metal(r,lit,reverse)) g.FillPath(fill,shape);
    }

    private static void Rivet(Graphics g,float x,float y) {
        var rect=new RectangleF(x-7,y-7,14,14);
        using(var shape=new GraphicsPath()) {
            shape.AddEllipse(rect);
            using(var brush=new PathGradientBrush(shape)) {
                brush.CenterPoint=new PointF(x-2,y-3);
                brush.CenterColor=Rgb(255,236,155);
                brush.SurroundColors=new []{Rgb(78,40,14)};
                g.FillEllipse(brush,rect);
            }
        }
        using(var pen=new Pen(Rgb(63,31,13),1.6f)) g.DrawEllipse(pen,rect);
    }

    private static Bitmap TrimAlpha(Bitmap source) {
        var area=new Rectangle(0,0,source.Width,source.Height);
        var data=source.LockBits(area,ImageLockMode.ReadOnly,PixelFormat.Format32bppArgb);
        int left=source.Width, top=source.Height, right=-1, bottom=-1;
        try {
            var pixels=new byte[data.Stride*source.Height];
            Marshal.Copy(data.Scan0,pixels,0,pixels.Length);
            for(int y=0;y<source.Height;y++) for(int x=0;x<source.Width;x++) {
                if(pixels[y*data.Stride+x*4+3]<=18) continue;
                left=Math.Min(x,left); top=Math.Min(y,top); right=Math.Max(x,right); bottom=Math.Max(y,bottom);
            }
        } finally { source.UnlockBits(data); }
        if (right<left || bottom<top) throw new InvalidDataException("Invalid ornament alpha");
        return source.Clone(Rectangle.FromLTRB(left,top,right+1,bottom+1),PixelFormat.Format32bppArgb);
    }

    private static void Fit(Graphics g,Bitmap image,RectangleF r) {
        double k=Math.Min(r.Width/image.Width,r.Height/image.Height);
        var w=(float)(image.Width*k); var h=(float)(image.Height*k);
        g.DrawImage(image,new RectangleF(r.X+(r.Width-w)*.5f,r.Y+(r.Height-h)*.5f,w,h));
    }

    private static void Corners(Graphics g,Part asset,Bitmap corner) {
        for(int j=0;j<2;j++) for(int i=0;i<2;i++) {
            GraphicsState transform=g.Save();
            g.TranslateTransform(i==0?0:asset.W,j==0?0:asset.H);
            g.ScaleTransform(i==0?1:-1,j==0?1:-1);
            Fit(g,corner,new RectangleF(7,6,93,93));
            g.Restore(transform);
        }
    }

    private static Bitmap Draw(Part asset,Bitmap crest,Bitmap corner) {
        var canvas=new Bitmap(asset.W,asset.H,PixelFormat.Format32bppArgb);
        using(var g=Graphics.FromImage(canvas)) {
            g.Clear(Color.Transparent);
            g.SmoothingMode=SmoothingMode.AntiAlias;
            g.InterpolationMode=InterpolationMode.HighQualityBicubic;
            var full=new RectangleF(0,0,asset.W,asset.H);
            string shape=asset.Shape;
            if(shape=="crest") {
                Fit(g,crest,Inset(full,3));
            } else if(shape=="left"||shape=="right") {
                bool left=shape=="left";
                PointF[] points={new PointF(left?10:38,24),new PointF(left?35:13,7),new PointF(left?35:13,41)};
                using(var metal=Metal(full,1)) using(var pen=new Pen(Rgb(87,47,16),2.1f)) {
                    g.FillPolygon(metal,points); g.DrawPolygon(pen,points);
                }
                using(var pen=new Pen(Rgb(255,230,133),1.5f))
                    g.DrawLine(pen,points[0],points[1]);
            } else if(shape=="glow") {
                for(int i=0;i<7;i++) {
                    using(var path=Outline("row",Inset(full,9+i)))
                    using(var pen=new Pen(Color.FromArgb(12+i*6,255,167,25),15f-i*1.6f))
                        g.DrawPath(pen,path);
                }
            } else if(shape=="divider") {
                var r=Inset(new RectangleF(0,3,asset.W,10),1);
                Brass(g,shape,r,.72);
            } else if(asset.Id=="row_normal") {
                // The normal rows share a tray, with a recessed seam between them.
                using(var tex=Leather(asset.W,asset.H,asset.Lit,false))
                using(var brush=new TextureBrush(tex,WrapMode.Clamp))
                    g.FillRectangle(brush, 3, 1, asset.W-6, asset.H-3);
                using(var pen=new Pen(Rgb(12,8,6),1f))
                    g.DrawLine(pen,3,asset.H-2,asset.W-4,asset.H-2);
                using(var pen=new Pen(Rgb(61,41,31),1f))
                    g.DrawLine(pen,3,1,asset.W-4,1);
            } else {
                bool wide=shape!="track"&&shape!="thumb";
                int pad=shape=="frame"?14:shape=="header"?6:wide?4:2;
                var r=Inset(full,pad);
                bool lit=asset.Lit>.65;
                if(lit && wide) for(int i=0;i<3;i++) {
                    using(var path=Outline(shape,Inset(r,-2+i)))
                    using(var pen=new Pen(Color.FromArgb(16+i*13,255,173,36),6-i*1.25f)) g.DrawPath(pen,path);
                }
                var shadow=new RectangleF(r.X,r.Y+2,r.Width,r.Height);
                FillOutline(g,shape,shadow,Color.FromArgb(180,10,4,1));
                Brass(g,shape,r,asset.Lit,asset.Id=="button_pressed");
                if(shape=="frame" || shape=="header") {
                    r=Inset(r,3);
                    FillOutline(g,shape,r,Rgb(61,29,12));
                    r=Inset(r,1);
                    Brass(g,shape,r,asset.Lit+.35,true);
                }
                float thickness=shape=="frame"?5:shape=="header"?2:shape=="button"?6:wide?3:2;
                if(lit && wide) {
                    r=Inset(r,1.2f);
                    FillOutline(g,shape,r,Rgb(255,222,120));
                    thickness-=.5f;
                }
                r=Inset(r,thickness);
                FillOutline(g,shape,r,lit?Rgb(128,60,14):Rgb(54,26,13));
                r=Inset(r,wide?2f:1f);
                if(shape=="thumb") {
                    Brass(g,shape,r,.87);
                } else {
                    using(var tex=Leather(asset.W,asset.H,asset.Lit,lit && shape!="track"))
                    using(var path=Outline(shape,r))
                    using(var brush=new TextureBrush(tex,WrapMode.Clamp)) g.FillPath(brush,path);
                }
                if(shape=="button") {
                    Rivet(g,27,asset.H*.49f);
                    Rivet(g,asset.W-27,asset.H*.49f);
                }
                if(shape=="frame") Corners(g,asset,corner);
            }
        }
        return canvas;
    }

    public static void Generate(string directory) {
        var rows=new StringBuilder();
        var declarations=new StringBuilder("// Generated by source/Generate.ps1\r\n");
        rows.AppendLine("{");
        rows.AppendLine("  \"schema\": 1,");
        rows.AppendLine("  \"format\": \"PNG RGBA8, straight alpha, sRGB\",");
        rows.AppendLine("  \"margins_order\": [\"left\", \"top\", \"right\", \"bottom\"],");
        rows.AppendLine("  \"composition\": \"Nine-slice in source pixels. Fixed corners scale with the UI; borders and center stretch. Zero margins use uniform asset scaling. Draw the selection halo before the selected row. Text is supplied by the host.\",");
        rows.AppendLine("  \"assets\": [");
        using(var rawCrest=new Bitmap(Path.Combine(directory,"source","crest-master.png")))
        using(var rawCorner=new Bitmap(Path.Combine(directory,"source","corner-master.png")))
        using(var crest=TrimAlpha(rawCrest))
        using(var corner=TrimAlpha(rawCorner))
        {
            for(int i=0;i<Parts.Length;i++) {
                Part p=Parts[i];
                string file=p.Id+".png";
                using(var bitmap=Draw(p,crest,corner)) bitmap.Save(Path.Combine(directory,file), ImageFormat.Png);
                rows.AppendLine($"    {{\"file\":\"{file}\",\"width\":{p.W},\"height\":{p.H},\"margins\":[{p.XSlice},{p.YSlice},{p.XSlice},{p.YSlice}],\"purpose\":\"{p.Purpose}\"}}"+(i+1==Parts.Length?"":","));
                declarations.AppendLine($"MAXIMO_F1_ASSET({p.Id}, \"{file}\", {p.W}, {p.H}, {p.XSlice}, {p.YSlice}, {p.XSlice}, {p.YSlice})");
            }
        }
        rows.AppendLine("  ]");
        rows.AppendLine("}");
        File.WriteAllText(Path.Combine(directory,"manifest.json"),rows.ToString());
        File.WriteAllText(Path.Combine(directory,"regions.inc"),declarations.ToString());
    }
}
