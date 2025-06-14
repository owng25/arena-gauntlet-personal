using System.Text.RegularExpressions;
using LibGit2Sharp;

var repoPath = ".";
if (args.Length == 2 && args[0] == "-p")
    repoPath = args[1];

var versionString = GetNewVersionString();

Console.WriteLine(versionString);

string GetNewVersionString()
{
    using var repo = new Repository(repoPath);
    var currentBranch = repo.Head;
    var isMain = currentBranch.FriendlyName.Equals("main");
    var branchSuffix = isMain ? string.Empty : $"-{currentBranch.FriendlyName}".Replace('/', '_');
    var regex = new Regex($"^\\d+.\\d+.\\d+({branchSuffix})?$");
    var latestTag = repo.Tags?.Where(t => regex.IsMatch(t.FriendlyName)).MaxBy(t => t, new VersionTagComparer());
    if (latestTag == null)
        throw new Exception("No version tags found");
    
    var latestCommit = repo.Head.Tip;
    var tagCommit = (Commit)latestTag.Target;

    var split = latestTag.FriendlyName.Split('.', '-');
    var (major, minor, patch) = (int.Parse(split[0]), int.Parse(split[1]), int.Parse(split[2]));

    // This is based on repo paths, it will break if the repo is reorganized
    var changes = repo.Diff.Compare<TreeChanges>(tagCommit.Tree, latestCommit.Tree);
    if (changes.Any(c => Regex.IsMatch(c.Path, "^schemas/.+\\.schema\\.json$")))
    {
        major++;
        minor = 0;
        patch = 0;
    }
    else if (changes.Any(c => Regex.IsMatch(c.Path, "^data/.+\\.json$")))
    {
        minor++;
        patch = 0;
    }
    else
    {
        patch++;
    }

    return $"{major}.{minor}.{patch}{branchSuffix}";
}

internal class VersionTagComparer : Comparer<Tag>
{
    public override int Compare(Tag? x, Tag? y)
    {
        // shouldn't have to deal with nullables here, but it complains if we don't
        var xParts = x?.FriendlyName.Split('.', '-') ?? new[] { "0", "0", "0" };
        var yParts = y?.FriendlyName.Split('.', '-') ?? new[] { "0", "0", "0" };

        var result = int.Parse(xParts[0]).CompareTo(int.Parse(yParts[0]));
        if (result != 0) return result;

        result = int.Parse(xParts[1]).CompareTo(int.Parse(yParts[1]));
        if (result != 0) return result;

        result = int.Parse(xParts[2]).CompareTo(int.Parse(yParts[2]));
        if (result != 0) return result;

        return yParts.Length - xParts.Length;
    }
}
