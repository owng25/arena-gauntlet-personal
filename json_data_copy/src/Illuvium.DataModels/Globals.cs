// a convenience class for holding globally required items

// This is needed to allow `init` syntax for record types in the assembly
// Seems to be an issue for using more advanced syntax while targeting older versions of .NET
// See https://developercommunity.visualstudio.com/t/error-cs0518-predefined-type-systemruntimecompiler/1244809
namespace System.Runtime.CompilerServices
{
    internal static class IsExternalInit { }
}