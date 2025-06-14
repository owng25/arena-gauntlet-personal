using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using Illuvium.DataModels.Json;
using NUnit.Framework;

namespace Illuvium.DataModels.Tests;

public class JsonDataTests
{
	private static IEnumerable<(string, string)>? _dataFiles;
	private static string? _resolvedDataDirectoryPath;

	// Relative path to the data directory from the test project's execution directory.
	// Common structure: ProjectFolder/bin/Debug/netX.X, so ../../../../../data implies data is 5 levels up from there.
	// Adjust if your "data" folder is located differently relative to your test execution path.
	private const string RelativeDataPath = "../../../../../data";

	private static string ResolvedDataDirectoryPath
	{
		get
		{
			if (_resolvedDataDirectoryPath == null)
			{
				string assemblyLocation = Path.GetDirectoryName(typeof(JsonDataTests).Assembly.Location) ?? AppDomain.CurrentDomain.BaseDirectory;
				string potentialBasePath = Path.Combine(assemblyLocation, RelativeDataPath);

				if (Directory.Exists(potentialBasePath))
				{
					_resolvedDataDirectoryPath = Path.GetFullPath(potentialBasePath);
				}
				else
				{
					// Fallback for scenarios where assembly location might not be ideal
					_resolvedDataDirectoryPath = Path.GetFullPath(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, RelativeDataPath));
				}

				if (!Directory.Exists(_resolvedDataDirectoryPath))
				{
					// Construct a more informative error message
					string attemptedPathMessage = $"Attempted to resolve from assembly location '{assemblyLocation}' to '{potentialBasePath}', and from base directory '{AppDomain.CurrentDomain.BaseDirectory}' to '{Path.GetFullPath(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, RelativeDataPath))}'.";
					throw new DirectoryNotFoundException($"The data directory could not be found using relative path '{RelativeDataPath}'. Current ResolvedDataDirectoryPath: '{_resolvedDataDirectoryPath}'. {attemptedPathMessage} Please check the RelativeDataPath constant and ensure the 'data' directory exists at the expected location relative to the test execution directory.");
				}
			}
			return _resolvedDataDirectoryPath;
		}
	}

	private static IEnumerable<(string path, string contents)> DataFiles
	{
		get
		{
			_dataFiles ??= LoadDataFiles().ToList();
			return _dataFiles;
		}
	}

	private static IEnumerable<(string path, string contents)> LoadDataFiles()
	{
		string basePath = ResolvedDataDirectoryPath; // Access property to ensure initialization and get path
		var files = Directory.EnumerateFiles(basePath, "*.json", SearchOption.AllDirectories)
			.Select(fileName => (Path.GetFullPath(fileName).Replace('\\', '/'), File.ReadAllText(fileName)));
		return files;
	}
	/// <summary>
	/// Helper method to determine if a file matches the given path criteria.
	/// </summary>
	/// <param name="fileInfo">A tuple containing the file's full path and its contents.</param>
	/// <param name="pathCriteria">The path string to match against (either a part of the path or an exact root folder).</param>
	/// <param name="matchExactRootFolder">If true, pathCriteria must match the root folder name; otherwise, pathCriteria is checked for containment in the full path.</param>
	/// <returns>True if the file matches the criteria, false otherwise.</returns>
	private static bool FileMatchesPathCriteria((string path, string contents) fileInfo, string pathCriteria, bool matchExactRootFolder)
	{
		if (matchExactRootFolder)
		{
			// Ensure ResolvedDataDirectoryPath ends with a directory separator for correct relative path calculation
			string normalizedBaseDir = ResolvedDataDirectoryPath.EndsWith(Path.DirectorySeparatorChar.ToString())
										? ResolvedDataDirectoryPath
										: ResolvedDataDirectoryPath + Path.DirectorySeparatorChar;

			// Ensure fileInfo.path is an absolute path before calling GetRelativePath
			string absoluteFilePath = Path.IsPathRooted(fileInfo.path) ? fileInfo.path : Path.GetFullPath(fileInfo.path);

			string relativePath = Path.GetRelativePath(normalizedBaseDir, absoluteFilePath);
			string normalizedRelativePath = relativePath.Replace('\\', '/');
			string? rootFolderOrFile = normalizedRelativePath.Split('/').FirstOrDefault();
			return !string.IsNullOrEmpty(rootFolderOrFile) && rootFolderOrFile.Equals(pathCriteria, StringComparison.OrdinalIgnoreCase);
		}
		return fileInfo.path.Contains(pathCriteria);
	}

	public static IEnumerable<TestCaseData> GetFile<T>(string path, bool matchExactRootFolder = false)
	{
		var file = DataFiles.Single(f => FileMatchesPathCriteria(f, path, matchExactRootFolder));
		yield return new TestCaseData(file.contents, typeof(T)).SetName($"{Path.GetFileName(file.path)} - {typeof(T).Name}");
	}

	public static IEnumerable<TestCaseData> GetFiles<T>(string path, bool matchExactRootFolder = false) => DataFiles
		.Where(f => FileMatchesPathCriteria(f, path, matchExactRootFolder))
		.Select(f => new TestCaseData(f.contents, typeof(T)).SetName($"{Path.GetFileName(f.path)} - {typeof(T).Name}"));

	public static IEnumerable<(string path, string contents)> GetFilesWithPath(string path, bool matchExactRootFolder = false) => DataFiles
		.Where(f => FileMatchesPathCriteria(f, path, matchExactRootFolder));

	// TestCaseSources - using original logic (matchExactRootFolder = false by default)
	public static IEnumerable<TestCaseData> CombatUnitFiles => GetFiles<CombatUnitType>("CombatUnitData");
	public static IEnumerable<TestCaseData> AffinityDataFiles => GetFiles<CombatAffinityType>("SynergyData/Affinity");
	public static IEnumerable<TestCaseData> CombatClassFiles => GetFiles<CombatClassType>("SynergyData/Class");
	public static IEnumerable<TestCaseData> HyperData => GetFile<HyperData>("HyperData/HyperData.json");
	public static IEnumerable<TestCaseData> CostData => GetFile<CostDataType>("CostData/Costs.json");
	public static IEnumerable<TestCaseData> HyperConfig => GetFile<HyperConfig>("HyperData/HyperConfig.json");
	public static IEnumerable<TestCaseData> WorldEffectsConfig => GetFile<WorldEffectsConfig>("WorldEffectsConfig.json");
	public static IEnumerable<TestCaseData> SuitFiles => GetFiles<CombatSuitType>("SuitData");
	public static IEnumerable<TestCaseData> WeaponFiles => GetFiles<CombatWeaponType>("WeaponData");
	public static IEnumerable<TestCaseData> AugmentFiles => GetFiles<AugmentType>("AugmentData", true);
	public static IEnumerable<TestCaseData> DroneAugmentFiles => GetFiles<DroneAugmentType>("DroneAugmentData");
	public static IEnumerable<TestCaseData> ConsumableFiles => GetFiles<ConsumableType>("ConsumableData");
	public static IEnumerable<TestCaseData> EncounterModFiles => GetFiles<EncounterModType>("EncounterModData");

	[TestCaseSource(nameof(CombatUnitFiles))]
	[TestCaseSource(nameof(AffinityDataFiles))]
	[TestCaseSource(nameof(CombatClassFiles))]
	[TestCaseSource(nameof(SuitFiles))]
	[TestCaseSource(nameof(WeaponFiles))]
	[TestCaseSource(nameof(CostData))]
	[TestCaseSource(nameof(HyperData))]
	[TestCaseSource(nameof(HyperConfig))]
	[TestCaseSource(nameof(WorldEffectsConfig))]
	[TestCaseSource(nameof(AugmentFiles))]
	[TestCaseSource(nameof(ConsumableFiles))]
	[TestCaseSource(nameof(EncounterModFiles))]
	[TestCaseSource(nameof(DroneAugmentFiles))]
	public void JsonToAndFromTests(string contents, Type type)
	{
		var unit = JsonSerializer.Deserialize(contents, type, JsonOptions.GameData);
		var json = JsonSerializer.Serialize(unit, JsonOptions.GameData);
		AssertJsonEquals(contents, json);
	}

	[Test]
	public void TestGeneratedDominantFields()
	{
		var combatUnitFiles = GetFilesWithPath("CombatUnitData"); // Default: matchExactRootFolder = false
		var combatUnitsLinesMap = new Dictionary<string, List<CombatUnitType>>();

		foreach (var file in combatUnitFiles)
		{
			var combatUnitType = JsonSerializer.Deserialize<CombatUnitType>(file.contents, JsonOptions.GameData);
			if (combatUnitType == null || string.IsNullOrEmpty(combatUnitType.Line))
			{
				// Log or handle missing Line property if necessary
				continue;
			}

			if (combatUnitType.UnitType != "Illuvial")
			{
				continue;
			}

			if (!combatUnitsLinesMap.ContainsKey(combatUnitType.Line))
			{
				combatUnitsLinesMap.Add(combatUnitType.Line, new List<CombatUnitType>());
			}
			combatUnitsLinesMap[combatUnitType.Line].Add(combatUnitType);
		}

		foreach (var (line, combatUnitTypes) in combatUnitsLinesMap)
		{
			CombatUnitType? stage1CombatUnit = GetCombatUnitDataForStage1SameLine(line, combatUnitsLinesMap);

			Assert.NotNull(stage1CombatUnit, $"Stage 1 of line '{line}' was not found. Check data integrity or GetCombatUnitDataForStage1SameLine logic.");
			if (stage1CombatUnit == null) // Should be caught by Assert.NotNull, but for safety in loops
			{
				continue;
			}

			foreach (var unitType in combatUnitTypes)
			{
				var expectedSynergies = GetExpectedDominantAffinityAndClass(stage1CombatUnit, unitType);
				Assert.AreEqual(expectedSynergies.dominantCombatAffinity, unitType.DominantCombatAffinity, $"Line = {line}, File = {Path.GetFileName(unitType.Path)}, Stage = {unitType.Stage} has invalid Dominant Affinity. Expected '{expectedSynergies.dominantCombatAffinity}', Got '{unitType.DominantCombatAffinity}'");
				Assert.AreEqual(expectedSynergies.dominantCombatClass, unitType.DominantCombatClass, $"Line = {line}, File = {Path.GetFileName(unitType.Path)}, Stage = {unitType.Stage} has invalid Dominant Class. Expected '{expectedSynergies.dominantCombatClass}', Got '{unitType.DominantCombatClass}'");
			}
		}
	}

	private static CombatUnitType? GetCombatUnitDataForStage1SameLine(string line, Dictionary<string, List<CombatUnitType>> combatUnitsLinesMap)
	{
		if (combatUnitsLinesMap.TryGetValue(line, out var unitsInLine))
		{
			foreach (var unit in unitsInLine)
			{
				if (unit.Stage == 1)
				{
					return unit;
				}
			}
		}
		return null; // Stage 1 not found in the given line
	}

	private static (string dominantCombatAffinity, string dominantCombatClass) GetExpectedDominantAffinityAndClass(CombatUnitType stage1CombatUnit, CombatUnitType combatUnit)
	{
		// Use rules from https://illuvium.atlassian.net/wiki/spaces/GD/pages/608206849/Dominant+Class

		// 1. Default with preferred

		string? dominantCombatAffinity = stage1CombatUnit.PreferredLineDominantCombatAffinity;
		string? dominantCombatClass = stage1CombatUnit.PreferredLineDominantCombatClass;

		// 2. Use Stage 1 Synergies if Preferred is invalid
		if (!IsSynergyValueValid(dominantCombatAffinity))
		{
			dominantCombatAffinity = stage1CombatUnit.CombatAffinity;
		}
		if (!IsSynergyValueValid(dominantCombatClass))
		{
			dominantCombatClass = stage1CombatUnit.CombatClass;
		}

		// Use combat unit synergies if all else fails
		if (!IsSynergyValueValid(dominantCombatAffinity))
		{
			dominantCombatAffinity = combatUnit.CombatAffinity;
		}
		if (!IsSynergyValueValid(dominantCombatClass))
		{
			dominantCombatClass = combatUnit.CombatClass;
		}
		return (dominantCombatAffinity ?? "None", dominantCombatClass ?? "None");
	}

	private static bool IsSynergyValueValid(string? synergyValue)
	{
		return !string.IsNullOrEmpty(synergyValue) && synergyValue != "None";
	}

	private static void AssertJsonEquals(string left, string right)
	{
		using JsonDocument leftDoc = JsonDocument.Parse(left);
		using JsonDocument rightDoc = JsonDocument.Parse(right);
		AssertEquals(leftDoc.RootElement, rightDoc.RootElement);
	}

	/// <summary>
	/// Deep compare for json elements. Taken from https://stackoverflow.com/a/60592310
	/// </summary>
	private static void AssertEquals(JsonElement x, JsonElement y)
	{
		Assert.AreEqual(x.ValueKind, y.ValueKind, $"ValueKind mismatch. Expected: {x.ValueKind}, Got: {y.ValueKind}. Left JSON: '{x.GetRawText()}', Right JSON: '{y.GetRawText()}'");

		switch (x.ValueKind)
		{
			case JsonValueKind.Null:
			case JsonValueKind.True:
			case JsonValueKind.False:
			case JsonValueKind.Undefined:
				return;

			// Compare the raw values of numbers, and the text of strings.
			// Note this means that 0.0 will differ from 0.00 -- which may be correct as deserializing either to `decimal` will result in subtly different results.
			// Newtonsoft's JValue.Compare(JTokenType valueType, object? objA, object? objB) has logic for detecting "equivalent" values,
			// you may want to examine it to see if anything there is required here.
			// https://github.com/JamesNK/Newtonsoft.Json/blob/master/Src/Newtonsoft.Json/Linq/JValue.cs#L246
			case JsonValueKind.Number:
				Assert.AreEqual(x.GetRawText(), y.GetRawText(), $"Number mismatch. Left: {x.GetRawText()}, Right: {y.GetRawText()}");
				break;

			case JsonValueKind.String:
				// Do not use GetRawText() here, it does not automatically resolve JSON escape sequences to their corresponding characters.
				Assert.AreEqual(x.GetString(), y.GetString(), $"String mismatch. Expected: '{x.GetString()}', Got: '{y.GetString()}'");
				break;

			case JsonValueKind.Array:
				var xList = x.EnumerateArray().ToList();
				var yList = y.EnumerateArray().ToList();
				Assert.AreEqual(xList.Count, yList.Count, $"Array length mismatch. Left count: {xList.Count}, Right count: {yList.Count}. Left: {x.GetRawText()}, Right: {y.GetRawText()}");
				for (int i = 0; i < xList.Count; i++)
				{
					AssertEquals(xList[i], yList[i]);
				}
				break;

			case JsonValueKind.Object:
				{
					// Surprisingly, JsonDocument fully supports duplicate property names.
					// I.e. it's perfectly happy to parse {"Value":"a", "Value" : "b"} and will store both
					// key/value pairs inside the document!
					// A close reading of https://www.rfc-editor.org/rfc/rfc8259#section-4 seems to indicate that
					// such objects are allowed but not recommended, and when they arise, interpretation of
					// identically-named properties is order-dependent.
					// So stably sorting by name then comparing values seems the way to go.
					var xProperties = x.EnumerateObject().ToList();
					var yProperties = y.EnumerateObject().ToList();

					var xPropertyNames = xProperties.Select(p => p.Name).OrderBy(name => name, StringComparer.Ordinal).ToList();
					var yPropertyNames = yProperties.Select(p => p.Name).OrderBy(name => name, StringComparer.Ordinal).ToList();

					bool countsMatch = xProperties.Count == yProperties.Count;

					var missingProperties = xPropertyNames.Except(yPropertyNames).ToList();
					var extraProperties = yPropertyNames.Except(xPropertyNames).ToList();

					if (!countsMatch || missingProperties.Any() || extraProperties.Any())
					{
						string errorMessage = $"Object property mismatch.{Environment.NewLine}";
						errorMessage += $"  Expected properties ({xPropertyNames.Count}): < {string.Join(", ", xPropertyNames.Select(p => $"\"{p}\""))} >{Environment.NewLine}";
						errorMessage += $"  Actual properties ({yPropertyNames.Count}):   < {string.Join(", ", yPropertyNames.Select(p => $"\"{p}\""))} >{Environment.NewLine}";

						if (missingProperties.Any())
						{
							errorMessage += $"  Missing properties ({missingProperties.Count}): < {string.Join(", ", missingProperties.Select(p => $"\"{p}\""))} >{Environment.NewLine}";
						}
						if (extraProperties.Any())
						{
							errorMessage += $"  Extra properties ({extraProperties.Count}): < {string.Join(", ", extraProperties.Select(p => $"\"{p}\""))} >{Environment.NewLine}";
						}
						// Optionally, include the raw JSON for context if it's not too verbose
						// errorMessage += $"Left JSON: {x.GetRawText()}{Environment.NewLine}";
						// errorMessage += $"Right JSON: {y.GetRawText()}{Environment.NewLine}";
						Assert.Fail(errorMessage.TrimEnd());
					}

					// If names and counts are fine, proceed to check values (order by name for consistent comparison)
					var xSortedProperties = xProperties.OrderBy(p => p.Name, StringComparer.Ordinal).ToList();
					var ySortedProperties = yProperties.OrderBy(p => p.Name, StringComparer.Ordinal).ToList();

					for (int i = 0; i < xSortedProperties.Count; i++)
					{
						var px = xSortedProperties[i];
						var py = ySortedProperties[i];
						// Name check is implicitly covered by the logic above if we reach here,
						// but an explicit Assert.AreEqual(px.Name, py.Name) could be added for super safety
						// though it would be redundant if the missing/extra logic is correct.
						AssertEquals(px.Value, py.Value);
					}
					break;
				}

			default:
				throw new JsonException($"Unknown JsonValueKind {x.ValueKind}");
		}
	}
}
