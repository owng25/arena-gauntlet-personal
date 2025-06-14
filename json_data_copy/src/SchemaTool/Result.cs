namespace SchemaTool;

public class Result
{
    public int TotalFiles { get; set; }
    public int TotalValidFiles { get; set; }
    public bool IsSuccess => TotalFiles == TotalValidFiles;

    public void Increment(Result other)
    {
        TotalFiles += other.TotalFiles;
        TotalValidFiles += other.TotalValidFiles;
    }
}